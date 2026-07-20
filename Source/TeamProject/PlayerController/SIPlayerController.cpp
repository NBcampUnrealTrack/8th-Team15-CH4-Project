// SIPlayerController.cpp


#include "PlayerController/SIPlayerController.h"
#include "Character/SICharacter.h"
#include "GameMode/SIGameMode.h"         
#include "GameState/SIGameState.h"
#include "PlayerState/SIPlayerState.h"
#include "UI/SIDrawingToolWidget.h"
#include "UI/SILobbySettingWidget.h"
#include "UI/SIHUDWidget.h"
#include "UI/SIParticipantsListWidget.h"
#include "UI/SIControlGuideWidget.h"
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

		// 채팅 기록은 위젯이 아니라 여기서 남긴다 (HUD가 없는 로비/결과 화면까지 커버)
		BindChatHistoryRecorder();
	}
}

void ASIPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GameState.IsValid())
	{
		GameState->OnPhaseChanged.RemoveDynamic(this, &ASIPlayerController::HandlePhaseChanged);
	}
	
	if (ChatRecorderBoundGameState.IsValid())
	{
		ChatRecorderBoundGameState->OnChatMessage.RemoveDynamic(
			this, &ASIPlayerController::HandleChatMessageForHistory);
	}

	if (UWorld* World = GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(GameStateRetryHandle);
		World->GetTimerManager().ClearTimer(ChatRecorderRetryTimer);
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
	// 인게임 — 참가자 명단(PlayerOrderList) 검증까지 포함된 기존 경로
	if (ASIGameMode* GM = GetWorld()->GetAuthGameMode<ASIGameMode>())
	{
		GM->OnChatReceived(this, Message);
		return;
	}

	// 로비 — ASILobbyGameMode는 ASIGameMode를 상속하지 않아 위 경로를 탈 수 없다.
	// (그대로 두면 로비에서 채팅이 조용히 사라진다)
	// GameState는 두 레벨이 공유하므로 여기서 직접 방송한다.
	// 로비엔 걸러낼 명단이 없고 접속해 있는 것 자체가 참가 자격이므로 길이만 정규화한다.
	ASIGameState* SIState = GetWorld()->GetGameState<ASIGameState>();
	if (!IsValid(SIState) || !IsValid(PlayerState))
	{
		return;
	}

	FString NormalizedMessage = Message.TrimStartAndEnd();
	if (NormalizedMessage.IsEmpty())
	{
		return;
	}
	NormalizedMessage.LeftInline(SIChatLimits::MaxMessageLength);

	FChatMessagePayload Payload;
	Payload.Sender = PlayerState;
	Payload.Message = NormalizedMessage;
	SIState->Multicast_BroadcastChatMessage(Payload);
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

void ASIPlayerController::RegisterChatWidget(USIUserWidget* ChatWidget)
{
	ActiveChatWidget = ChatWidget;
}

void ASIPlayerController::UnregisterChatWidget(const USIUserWidget* ChatWidget)
{
	// 새 위젯이 이미 등록을 마친 뒤에 옛 위젯이 소멸할 수 있다.
	// 자기가 등록한 것일 때만 지워야 방금 등록된 위젯을 날리지 않는다.
	if (ActiveChatWidget.Get() == ChatWidget)
	{
		ActiveChatWidget.Reset();
	}
}

void ASIPlayerController::BindChatHistoryRecorder()
{
	ASIGameState* SIState = GetWorld() ? GetWorld()->GetGameState<ASIGameState>() : nullptr;
	if (!IsValid(SIState))
	{
		// 클라이언트에선 GameState 복제가 PC보다 늦을 수 있다 — 이 코드베이스의 0.1초 재시도 패턴
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				ChatRecorderRetryTimer, this, &ASIPlayerController::BindChatHistoryRecorder, 0.1f, false);
		}
		return;
	}

	ChatRecorderBoundGameState = SIState;
	SIState->OnChatMessage.AddDynamic(this, &ASIPlayerController::HandleChatMessageForHistory);
}

void ASIPlayerController::HandleChatMessageForHistory(const FChatMessagePayload& Payload)
{
	// Sender는 APlayerState* 라 레벨 이동을 넘기면 전부 null이 된다.
	// 받는 즉시 이름을 문자열로 박제해야 travel 후에도 "???"가 되지 않는다.
	// (시스템 안내는 빈 이름으로 저장되어 복원 시에도 본문만 그려진다)
	const FString SenderName = USIUserWidget::ResolveChatSenderName(Payload);

	if (USIGameInstance* GI = GetGameInstance<USIGameInstance>())
	{
		GI->AddChatLog(SenderName, Payload.Message);
	}
}

void ASIPlayerController::FocusChat()
{
	// 이미 채팅 중이거나 화면에 채팅창이 없으면 무시
	if (bChatFocused || !ActiveChatWidget.IsValid())
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
			if (bChatFocused && ActiveChatWidget.IsValid())
			{
				ActiveChatWidget->FocusChatInput();
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

	// 조작 가이드: F1을 누르고 있는 동안만 표시.
	// Tab(참가자 목록)처럼 IA 애셋을 두지 않고 Enter와 같이 키를 직접 잡는다 —
	// 홀드 표시에 필요한 건 눌림/뗌 두 시점뿐이라 애셋을 추가할 이유가 없다.
	InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ASIPlayerController::OpenControlGuideWidget);
	InputComponent->BindKey(EKeys::F1, IE_Released, this, &ASIPlayerController::CloseControlGuideWidget);

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
	if (CurrentPhase != ESIGamePhase::ResultPhase)
	{
		return;
	}

	if (IsValid(ScoreBoardWidget))
	{
		ScoreBoardWidget->RemoveFromParent();
	}

	// 점수판을 닫은 자리에 빈 게임 화면이 드러나지 않도록 곧바로 로딩 화면을 덮는다.
	//
	// 이건 순전히 로컬 연출이다. 리슨 서버에서는 클라이언트가 서버와 다른 맵에 있을 수 없어
	// 혼자 먼저 로비로 갈 방법이 없고(복귀 전까지 로비 맵은 존재하지도 않는다),
	// 실제 이동은 서버가 전원을 동시에 옮긴다.
	// 그래서 여기서 할 수 있는 건 "내 화면만 먼저 넘긴다"까지다.
	if (USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>())
	{
		SIInstance->ShowLoadingScreen();
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

/** 이 단계에서 F1 조작 가이드를 띄울 수 있는가.
	여는 조건과 "단계가 바뀌면 걷어내는" 조건이 반드시 같아야 해서 한 곳에 둔다.
	(한쪽만 고치면 가이드가 없는 단계로 넘어갔을 때 영영 안 닫히거나,
	 띄울 수 있는 단계인데 진입하자마자 사라지는 버그가 난다)
	로비: 캐릭터를 조작해 돌아다닐 수 있으므로 안내할 조작이 있다.
	결과 화면: 순위만 보는 화면이라 제외. */
static bool IsControlGuideAvailableInPhase(const ESIGamePhase Phase)
{
	return Phase == ESIGamePhase::LobbyPhase
		|| Phase == ESIGamePhase::BuildPhase
		|| Phase == ESIGamePhase::GuessPhase;
}

void ASIPlayerController::OpenControlGuideWidget()
{
	if (!IsControlGuideAvailableInPhase(CurrentPhase))
	{
		return;
	}

	// 키 반복 입력으로 Pressed가 연달아 들어와도 위젯이 겹쳐 쌓이지 않게 막는다
	if (IsValid(ControlGuideWidget) || !ControlGuideWidgetClass)
	{
		return;
	}

	ControlGuideWidget = CreateWidget<USIControlGuideWidget>(this, ControlGuideWidgetClass);
	if (!IsValid(ControlGuideWidget))
	{
		return;
	}

	// HUD/드로잉툴(기본 0) 위에 얹는다. 단계가 바뀌며 HUD가 새로 생성돼도 가려지지 않도록
	// 명시적인 ZOrder를 준다. 로딩 화면(1000)보다는 아래.
	constexpr int32 ControlGuideZOrder = 100;
	ControlGuideWidget->AddToViewport(ControlGuideZOrder);
}

void ASIPlayerController::CloseControlGuideWidget()
{
	// 여는 쪽과 달리 페이즈를 따지지 않는다.
	// 누른 채로 단계가 넘어가면 조건이 어긋나 영영 안 닫히기 때문이다.
	if (!IsValid(ControlGuideWidget))
	{
		return;
	}

	ControlGuideWidget->RemoveFromParent();
	ControlGuideWidget = nullptr;
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

	// F1을 누른 채로 단계가 넘어갈 수 있다. 가이드를 띄울 수 있는 단계끼리의 이동이면 그대로 두고,
	// 없는 단계로 갔다면 뗄 때까지 기다리지 않고 걷어낸다.
	// (CloseAllPhaseWidgets에 넣지 않는 건 제작→정답에서 깜빡이지 않게 하기 위해서다)
	if (!IsControlGuideAvailableInPhase(CurrentPhase))
	{
		CloseControlGuideWidget();
	}

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
		if (IsValid(ResultSFX))
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ResultSFX);
		}
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
	case ESIGamePhase::ResultPhase:
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
