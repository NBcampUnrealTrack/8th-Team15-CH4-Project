// SIPlayerController.cpp


#include "PlayerController/SIPlayerController.h"
#include "Character/SICharacter.h"
#include "GameMode/SIGameMode.h"         
#include "GameState/SIGameState.h"        
#include "UI/SIDrawingToolWidget.h"
#include "UI/SILobbySettingWidget.h"
#include "UI/SIHUDWidget.h"
#include "UI/SIParticipantsListWidget.h"
#include "UI/SIScoreBoardWidget.h"

#include "EnhancedInputComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Engine/GameViewportClient.h"
#include "GameInstance/SIGameInstance.h"
#include "Kismet/GameplayStatics.h"

ASIPlayerController::ASIPlayerController()
{
	BGMAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BGMAudioComponent"));
	BGMAudioComponent->bAutoActivate = false;
}

void ASIPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("=== [DEBUG] PC BeginPlay Started ==="));
		
		//레벨에 따른 BGM 초기화
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ASIPlayerController::InitializeLevelBGM, 0.5f, false);
		
		TryCacheGameState();
	}
}

void ASIPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GameState.IsValid())
	{
		GameState->OnPhaseChanged.RemoveDynamic(this, &ASIPlayerController::HandlePhaseChanged);
	}
	
	if (UWorld* World = GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(GameStateRetryHandle);
	}
	
	//사운드 재생 제거
	if (BGMAudioComponent)
	{
		BGMAudioComponent->Stop();
	}
	Super::EndPlay(EndPlayReason);
}


#pragma region GameMode

// ==========================================
// [Client -> Server] 정답 제출 실제 구현부
// ==========================================
void ASIPlayerController::Server_SubmitAnswer_Implementation(const FString& Answer)
{
	if (HasAuthority())
	{
		ASIGameMode* GameMode = GetWorld()->GetAuthGameMode<ASIGameMode>();
		if (GameMode)
		{
			GameMode->OnAnswerSubmitted(this, Answer);
		}
	}
}

// ==========================================
// [Server -> Client] 제시어 수신 실제 구현부
// ==========================================
void ASIPlayerController::Client_ReceiveSecretWord_Implementation(const FString& SecretWord)
{
	CachedSecretWord = SecretWord;
	
	if (HUDWidget)
	{
		HUDWidget->SetSecretWord(SecretWord);
	}
}

// ==========================================
// [테스트 콘솔 명령어] 
// ==========================================
void ASIPlayerController::TestAnswer(const FString& Answer)
{
	Server_SubmitAnswer(Answer);
}

void ASIPlayerController::SetPhase(int32 PhaseIndex)
{
	Server_TestSetPhase(PhaseIndex);
}

void ASIPlayerController::SetTime(int32 Seconds)
{
	Server_TestSetTime(Seconds);
}

void ASIPlayerController::Server_SendChat_Implementation(const FString& Message)
{
	if (ASIGameMode* GM = GetWorld()->GetAuthGameMode<ASIGameMode>())
	{
		GM->OnChatReceived(this, Message);
	}
}

// ==========================================
// [테스트 RPC] 인코딩 에러 방지를 위해 영어 로그 사용
// ==========================================
void ASIPlayerController::Server_TestSetPhase_Implementation(int32 PhaseIndex)
{
	if (HasAuthority())
	{
		ASIGameState* SIGameState = GetWorld()->GetGameState<ASIGameState>();
		if (SIGameState)
		{
			switch (PhaseIndex)
			{
			case 1:
				SIGameState->SetGamePhase(ESIGamePhase::BuildPhase);
				break;
			case 2:
				SIGameState->SetGamePhase(ESIGamePhase::GuessPhase);
				break;
			default:
				SIGameState->SetGamePhase(ESIGamePhase::None);
				break;
			}
		}
	}
}

void ASIPlayerController::Server_TestSetTime_Implementation(int32 Seconds)
{
	if (HasAuthority())
	{
		ASIGameState* SIGameState = GetWorld()->GetGameState<ASIGameState>();
		if (SIGameState)
		{
			SIGameState->SetRemainingTime(Seconds);
		}
	}
}

#pragma endregion

#pragma region UI

void ASIPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	if (IsLocalController())
	{
		// 게임 시작 즉시 뷰포트가 마우스를 잡도록 초기 입력 상태를 설정한다.
		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);

		if (UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport())
		{
			GameViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
			GameViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
		}
	}

}

void ASIPlayerController::FocusChat()
{
	// 이미 채팅 중이거나 HUD가 없으면 무시
	if (bChatFocused || !IsValid(HUDWidget))
	{
		return;
	}

	bChatFocused = true;

	// UI가 키보드 입력을 받을 수 있도록 GameAndUI로 전환 + 커서 표시
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	if (UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport())
	{
		GameViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
		GameViewportClient->SetMouseLockMode(EMouseLockMode::DoNotLock);
	}

	// 이번 Enter 입력이 소진된 다음 프레임에 포커스를 줘서, 열자마자 빈 커밋으로 닫히는 걸 방지
	GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (bChatFocused && IsValid(HUDWidget))
			{
				HUDWidget->FocusChatInput();
			}
		}));
}

void ASIPlayerController::EndChatFocus()
{
	if (!bChatFocused)
	{
		return;
	}

	bChatFocused = false;

	// 다시 게임 전용 입력으로 복귀
	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	if (UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport())
	{
		GameViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		GameViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
	}
}

void ASIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// 채팅 열기: Enter 키를 직접 바인딩(별도 IA 애셋 불필요).
	// GameOnly 모드에서 Enter가 게임 입력으로 들어올 때 채팅창 포커스로 전환한다.
	// (채팅창이 포커스된 동안엔 Enter를 Slate(EditableText)가 먹으므로 여기로 안 옴)
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ASIPlayerController::FocusChat);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	if (!EnhancedInputComponent)
	{
		return;
	}

	EnhancedInputComponent->BindAction(
		IA_ScoreBoardWidgetCancel, 
		ETriggerEvent::Started,
		this,
		&ASIPlayerController::RemovedScoreBoardWidget
		);
	EnhancedInputComponent->BindAction(
		IA_ParticipantsListWidgetCancel, 
		ETriggerEvent::Started, 
		this, 
		&ASIPlayerController::OpenParticipantsListWidget
		);
	EnhancedInputComponent->BindAction(
		IA_ParticipantsListWidgetCancel, 
		ETriggerEvent::Completed, 
		this, 
		&ASIPlayerController::CloseParticipantsListWidget
		);
}

void ASIPlayerController::RemovedScoreBoardWidget()
{
	if (CurrentPhase == ESIGamePhase::ResultPhase)
	{
		if (IsValid(ScoreBoardWidget))
		{
			ScoreBoardWidget->RemoveFromParent();
		}
	}
}

void ASIPlayerController::OpenParticipantsListWidget()
{
	if (CurrentPhase != ESIGamePhase::None && CurrentPhase != ESIGamePhase::ResultPhase)
	{
		if (ParticipantsListWidgetClass)
		{
			ParticipantsListWidget = CreateWidget<USIParticipantsListWidget>(this, ParticipantsListWidgetClass);
		
			ParticipantsListWidget->AddToViewport();
			
			UGameplayStatics::PlaySound2D(GetWorld(), TabOpenSound);
		}
	}
}

void ASIPlayerController::CloseParticipantsListWidget()
{
	if (ParticipantsListWidget)
	{
		ParticipantsListWidget->RemoveFromParent();
		
		UGameplayStatics::PlaySound2D(GetWorld(), TabCloseSound);
	}
}

void ASIPlayerController::CloseAllPhaseWidgets()
{
	if (LobbySettingWidget)
	{
		LobbySettingWidget->RemoveFromParent();
		LobbySettingWidget = nullptr;
	}
	
	if (DrawingToolWidget)
	{
		DrawingToolWidget->OnColorSelected.RemoveDynamic(this, &ThisClass::HandleDrawingToolColorSelected);
		DrawingToolWidget->RemoveFromParent();
		DrawingToolWidget = nullptr;
	}
	
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}
	
	if (ScoreBoardWidget)
	{
		ScoreBoardWidget->RemoveFromParent();
		ScoreBoardWidget = nullptr;
	}
}

void ASIPlayerController::HandlePhaseChanged(ESIGamePhase NewPhase)
{
	CurrentPhase = NewPhase;
	
	//사운드 파라미터 업데이트 함수
	UpdateBGMParameters(NewPhase);
	
	CloseAllPhaseWidgets();

	if (ASICharacter* SICharacter = Cast<ASICharacter>(GetPawn()))
	{
		SICharacter->HandleShapeEditingPhaseChanged(NewPhase);
	}
	
	switch (NewPhase)
	{
	
	case ESIGamePhase::None:
		break;
		
	case ESIGamePhase::LobbyPhase:
		if (LobbySettingWidgetClass)
		{
			LobbySettingWidget = CreateWidget<USILobbySettingWidget>(this, LobbySettingWidgetClass);
			LobbySettingWidget->AddToViewport();
		}
		break;

	case ESIGamePhase::BuildPhase:
		if (HUDWidgetClass && DrawingToolWidgetClass)
		{
			HUDWidget = CreateWidget<USIHUDWidget>(this, HUDWidgetClass);
			DrawingToolWidget = CreateWidget<USIDrawingToolWidget>(this, DrawingToolWidgetClass);

			// 플레이어 컨트롤러가 UI 이벤트를 현재 소유 캐릭터의 편집 상태로 전달한다.
			DrawingToolWidget->OnColorSelected.AddUniqueDynamic(this, &ThisClass::HandleDrawingToolColorSelected);

			HUDWidget->SetSecretWord(CachedSecretWord);
			
			HUDWidget->AddToViewport();
			DrawingToolWidget->AddToViewport();
		}
		break;

	case ESIGamePhase::GuessPhase:
		if (HUDWidgetClass)
		{
			HUDWidget = CreateWidget<USIHUDWidget>(this, HUDWidgetClass);
			HUDWidget->AddToViewport();
		}
		break;

	case ESIGamePhase::ResultPhase:
		if (ScoreBoardWidgetClass)
		{
			ScoreBoardWidget = CreateWidget<USIScoreBoardWidget>(this, ScoreBoardWidgetClass);
			ScoreBoardWidget->AddToViewport();
		}
		break;
	}
	
}

void ASIPlayerController::HandleDrawingToolColorSelected(int32 ColorIndex, FLinearColor Color)
{
	if (ASICharacter* SICharacter = Cast<ASICharacter>(GetPawn()))
	{
		SICharacter->SetSelectedPaletteColor(ColorIndex, Color);
	}
}

void ASIPlayerController::TryCacheGameState()
{
	ASIGameState* GS = GetWorld()->GetGameState<ASIGameState>();
	if (!IsValid(GS))
	{
		GameStateRetryHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASIPlayerController::TryCacheGameState);
		return;
	}
	
	GameState = GS;
	
	if (!GameState.IsValid())
	{
		return;
	}
	
	GameState->OnPhaseChanged.AddDynamic(this, &ASIPlayerController::HandlePhaseChanged);
	HandlePhaseChanged(GS->CurrentGamePhase);
}

void ASIPlayerController::StartRoomCreation()
{
	Cast<USIGameInstance>(GetGameInstance())->CreateRoom();
}

void ASIPlayerController::OpenLobbySettingWidget()
{
	if (!LobbySettingWidgetClass)
	{
		return;
	}

	LobbySettingWidget = CreateWidget<USILobbySettingWidget>(this, LobbySettingWidgetClass);

	LobbySettingWidget->AddToViewport();
}

void ASIPlayerController::CloseLobbySettingWidget()
{
	if (!IsValid(LobbySettingWidget))
	{
		return;
	}

	LobbySettingWidget->RemoveFromParent();
}


#pragma endregion

#pragma region Sound

void ASIPlayerController::UpdateBGMParameters(ESIGamePhase NewPhase)
{
	//지금 재생 중인 사운드가 메타 사운드 일때만 파라미터 업데이트 하도록 하였음
	if (!BGMAudioComponent || !BGMAudioComponent->IsPlaying()) return;
	if (BGMAudioComponent->GetSound() != BGMMetaSound) return;
 
	float PhaseValue = 0.0f;
 
	switch (NewPhase)
	{
	case ESIGamePhase::BuildPhase:
		PhaseValue = 0.0f; // 작업 음악 (In 0)
		break;
	case ESIGamePhase::GuessPhase:
		PhaseValue = 1.0f; // 정답 음악 (In 1)
		break;
	default:
		// 다른 페이즈일 때의 기본값 설정
		PhaseValue = 0.0f; 
		break;
	}
	
	//메타 사운드 내부 phase 파라미터 값 업데이트
	BGMAudioComponent->SetFloatParameter(FName("Phase"), PhaseValue);
}

void ASIPlayerController::InitializeLevelBGM()
{
	if (!IsValid(BGMAudioComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("BGMAudioComponent was NULL. Creating it at runtime as a fallback."));
		return; 
	}
	
	BGMAudioComponent = NewObject<UAudioComponent>(this, TEXT("RuntimeBGMAudioComponent"));
	
	if (BGMAudioComponent)
	{
		BGMAudioComponent->RegisterComponent(); // 런타임 생성 시 필수 호출
		BGMAudioComponent->bAutoActivate = false;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create BGMAudioComponent at runtime!"));
		return;
	}
	
	if (!IsValid(BGMMetaSound))
	{
		UE_LOG(LogTemp, Warning, TEXT("BGM Warning: BGMMetaSound is not assigned in Blueprint."));
		return;
	}
	
	//현재 레벨 이름 가져오기
	FString MapName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	
	UE_LOG(LogTemp, Warning, TEXT("Current Map Name: [%s]"), *MapName);
	
	USoundBase* SoundToPlay = nullptr;
	
	//레벨에 따라 사운드 재생
	if (MapName.Contains(TEXT("Test_Lobby")))
	{
		SoundToPlay = LobbyBGM;
	}
	else if (MapName.Contains(TEXT("MainLevel")))
	{
		SoundToPlay = BGMMetaSound;
	}	
	
	//사운드 교체 및 실행
	if (IsValid(SoundToPlay))
	{
		BGMAudioComponent->Stop(); 
		BGMAudioComponent->SetSound(SoundToPlay);
		BGMAudioComponent->Play();
	}
}

#pragma endregion 
