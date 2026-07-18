// Fill out your copyright notice in the Description page of Project Settings.


#include "SITitlePlayerController.h"

#include "EnhancedInputComponent.h"
#include "Component/SIUIManagerComponent.h"
#include "GameInstance/SIGameInstance.h"
#include "UI/SICreateRoomWidget.h"
#include "UI/SIMainMenuWidget.h"

ASITitlePlayerController::ASITitlePlayerController()
{
	UIManagerComponent = CreateDefaultSubobject<USIUIManagerComponent>(TEXT("UIManagerComponent"));
	
	BGMAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BGMAudioComponent"));
	BGMAudioComponent->bAutoActivate = false;
}

void ASITitlePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	// 메인메뉴 띄우고 바인딩하는 코드. 결과물 합치고 난 이후에 주석 해제하기.
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	SetInputMode(InputMode);
	
	if (IsLocalController())
	{
		// 0.2초 뒤에 메인메뉴 음악 실행 (안전장치)
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ASITitlePlayerController::InitializeMainMenuBGM, 0.2f, false);
		
	}

	if (!IsLocalController())
	{
		return;
	}

	UIManagerComponent->OnCreateRoomRequested.AddDynamic(this, &ASITitlePlayerController::OnCreateRoomClicked);

	TObjectPtr<USIMainMenuWidget> MainMenuWidget = CreateWidget<USIMainMenuWidget>(this, MainMenuWidgetClass);

	if (!IsValid(MainMenuWidget))
	{
		return;
	}

	MainMenuWidget->OnClickedCreateRoomButton.AddDynamic(this, &ASITitlePlayerController::HandleCreateRoom);
	MainMenuWidget->OnClickedJoinRoomButton.AddDynamic(this, &ASITitlePlayerController::HandleJoinRoom);
	MainMenuWidget->OnClickedQuitButton.AddDynamic(this, &ASITitlePlayerController::HandleQuit);

	MainMenuWidget->AddToViewport();
}

void ASITitlePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	if (!EnhancedInputComponent)
	{
		return;
	}

	EnhancedInputComponent->BindAction(IA_UICancel, ETriggerEvent::Started, this, &ASITitlePlayerController::HandleCancel);
}

void ASITitlePlayerController::HandleCancel()
{
	UIManagerComponent->CloseWidget();
}

void ASITitlePlayerController::HandleCreateRoom()
{
	UIManagerComponent->OpenWidget(EUIType::CreateRoom);
}

void ASITitlePlayerController::HandleJoinRoom()
{
	UIManagerComponent->OpenWidget(EUIType::RoomList);
}

void ASITitlePlayerController::HandleQuit()
{
	UIManagerComponent->OpenWidget(EUIType::Exit);
}

void ASITitlePlayerController::OnCreateRoomClicked(const FSICreateSessionParams& Settings)
{
	Cast<USIGameInstance>(GetGameInstance())->CreateRoom(Settings);
	UE_LOG(LogTemp, Warning, TEXT("Called OnCreateRoomClicked"));
}
#pragma region Sound

void ASITitlePlayerController::InitializeMainMenuBGM()
{
	// 크래시 방지: IsValid 체크 필수
	if (IsValid(BGMAudioComponent) && IsValid(MainMenuBGM))
	{
		BGMAudioComponent->SetSound(MainMenuBGM);
		BGMAudioComponent->Play();
		UE_LOG(LogTemp, Warning, TEXT("Title BGM Started!"));
	}
}

#pragma endregion
