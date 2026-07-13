// SIPlayerController.cpp


#include "PlayerController/SIPlayerController.h"
#include "GameMode/SIGameMode.h"         
#include "GameState/SIGameState.h"        
#include "UI/SIDrawingToolWidget.h"
#include "UI/SILobbySettingWidget.h"
#include "UI/SIHUDWidget.h"
#include "UI/SIParticipantsListWidget.h"
#include "UI/SIScoreBoardWidget.h"

#include "EnhancedInputComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameInstance/SIGameInstance.h"

ASIPlayerController::ASIPlayerController()
{

}

void ASIPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
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
			UE_LOG(LogTemp, Warning, TEXT("[서버] 클라이언트가 제출한 단어 수신: %s"), *Answer);
		}
	}
}

// ==========================================
// [Server -> Client] 제시어 수신 실제 구현부
// ==========================================
void ASIPlayerController::Client_ReceiveSecretWord_Implementation(const FString& SecretWord)
{
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
	UE_LOG(LogTemp, Warning, TEXT("[클라이언트] 당신의 이번 턴 출제 제시어는 [%s] 입니다!"), *SecretWord);
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
	
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
	UE_LOG(LogTemp, Warning, TEXT("콘솔에서 정답 제출 시도 중... 제출 단어: %s"), *Answer);
	Server_SubmitAnswer(Answer);
}

void ASIPlayerController::SetPhase(int32 PhaseIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("[테스트] 페이즈 변경 명령 전송: %d"), PhaseIndex);
	Server_TestSetPhase(PhaseIndex);
}

void ASIPlayerController::SetTime(int32 Seconds)
{
	UE_LOG(LogTemp, Warning, TEXT("[테스트] 타이머 변경 명령 전송: %d초"), Seconds);
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
				SIGameState->CurrentGamePhase = ESIGamePhase::BuildPhase;
				UE_LOG(LogTemp, Warning, TEXT("[Server] Changed to BuildPhase"));
				break;
			case 2:
				SIGameState->CurrentGamePhase = ESIGamePhase::GuessPhase;
				UE_LOG(LogTemp, Warning, TEXT("[Server] Changed to GuessPhase"));
				break;
			default:
				SIGameState->CurrentGamePhase = ESIGamePhase::None;
				UE_LOG(LogTemp, Warning, TEXT("[Server] Changed to None"));
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
			SIGameState->RemainingTime = Seconds;
			UE_LOG(LogTemp, Warning, TEXT("[Server] Timer set to %d seconds"), Seconds);
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

void ASIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
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
		}
	}
}

void ASIPlayerController::CloseParticipantsListWidget()
{
	if (ParticipantsListWidget)
	{
		ParticipantsListWidget->RemoveFromParent();
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
	CloseAllPhaseWidgets();
	
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

	OpenLobbySettingWidget();
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
