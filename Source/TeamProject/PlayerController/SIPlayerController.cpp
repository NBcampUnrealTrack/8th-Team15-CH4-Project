// SIPlayerController.cpp


#include "PlayerController/SIPlayerController.h"
#include "Component//SIUIManagerComponent.h"
#include "GameMode/SIGameMode.h"         
#include "GameState/SIGameState.h"        
#include "UI/SIDrawingToolWidget.h" 
#include "UI/SIMainMenuWidget.h"
#include "UI/SILobbySettingWidget.h"

#include "EnhancedInputComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameInstance/SIGameInstance.h"

ASIPlayerController::ASIPlayerController()
{
	UIManagerComponent = CreateDefaultSubobject<USIUIManagerComponent>(TEXT("UIManagerComponent"));
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

void ASIPlayerController::HandleCreateRoom()
{
	UIManagerComponent->OpenWidget(EUIType::CreateRoom);
}

void ASIPlayerController::HandleJoinRoom()
{
	UIManagerComponent->OpenWidget(EUIType::RoomList);
}

void ASIPlayerController::HandleQuit()
{
	UIManagerComponent->OpenWidget(EUIType::Exit);
}

void ASIPlayerController::HandleCreateRoomConfirmed()
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

void ASIPlayerController::HandleUIConfirmed(EUIType type)
{
	UIManagerComponent->OpenWidget(type);
}

void ASIPlayerController::HandleCancel()
{
	UIManagerComponent->CloseWidget();
}

void ASIPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	//인스턴스가 없을 때, StaticClass만 존재한다면
	if (!DrawingToolWidgetInstance && DrawingToolWidget)
	{
		// StaticClass를 통해 Instance화
		DrawingToolWidgetInstance = CreateWidget<USIDrawingToolWidget>(this, DrawingToolWidget);
	}
	
	// 인스턴스가 존재한다면
	if (DrawingToolWidgetInstance)
	{
		// 뷰포트에 노출
		DrawingToolWidgetInstance->AddToViewport();
	}

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
	
	// // 메인메뉴 띄우고 바인딩하는 코드. 결과물 합치고 난 이후에 주석 해제하기.
	// bShowMouseCursor = true;
	//
	// FInputModeGameAndUI InputMode;
	// SetInputMode(InputMode);
	//
	// if (!IsLocalController())
	// {
	// 	return;
	// }
	//
	// UIManagerComponent->OnUIConfirmed.AddDynamic(this, &ASIPlayerController::HandleUIConfirmed);
	// UIManagerComponent->OnCreateRoomRequested.AddDynamic(this, &ASIPlayerController::OnCreateRoomClicked);
	//
	// TObjectPtr<USIMainMenuWidget> MainMenuWidget = CreateWidget<USIMainMenuWidget>(this, MainMenuWidgetClass);
	//
	// if (!IsValid(MainMenuWidget))
	// {
	// 	return;
	// }
	//
	// MainMenuWidget->OnClickedCreateRoomButton.AddDynamic(this, &ASIPlayerController::HandleCreateRoom);
	// MainMenuWidget->OnClickedJoinRoomButton.AddDynamic(this, &ASIPlayerController::HandleJoinRoom);
	// MainMenuWidget->OnClickedQuitButton.AddDynamic(this, &ASIPlayerController::HandleQuit);
	//
	// MainMenuWidget->AddToViewport();
}

void ASIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	if (!EnhancedInputComponent)
	{
		return;
	}

	EnhancedInputComponent->BindAction(IA_UICancel, ETriggerEvent::Started, this, &ASIPlayerController::HandleCancel);
}

#pragma endregion
