// Fill out your copyright notice in the Description page of Project Settings.


#include "SITitlePlayerController.h"

#include "EnhancedInputComponent.h"
#include "Component/SIUIManagerComponent.h"
#include "GameInstance/SIGameInstance.h"
#include "UI/SICreateRoomWidget.h"
#include "UI/SIMainMenuWidget.h"
#include "UI/SINoticeWidget.h"

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

	// 생성 성공 시 로비를 여는 건 GameInstance 몫. 여기선 실패 안내만 담당한다.
	if (USISessionSubsystem* Subsystem = GetGameInstance()->GetSubsystem<USISessionSubsystem>())
	{
		Subsystem->OnCreateSessionCompleteEvent.AddUniqueDynamic(
			this, &ASITitlePlayerController::HandleCreateSessionResult);
	}

	TObjectPtr<USIMainMenuWidget> MainMenuWidget = CreateWidget<USIMainMenuWidget>(this, MainMenuWidgetClass);

	if (!IsValid(MainMenuWidget))
	{
		return;
	}

	MainMenuWidget->OnClickedCreateRoomButton.AddDynamic(this, &ASITitlePlayerController::HandleCreateRoom);
	MainMenuWidget->OnClickedJoinRoomButton.AddDynamic(this, &ASITitlePlayerController::HandleJoinRoom);
	MainMenuWidget->OnClickedQuitButton.AddDynamic(this, &ASITitlePlayerController::HandleQuit);

	MainMenuWidget->AddToViewport();

	// 메인메뉴가 올라온 뒤에 안내창을 얹어야 스택 순서가 맞다
	ShowPendingFailureNotice();
}

void ASITitlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstanceRef = GetGameInstance())
	{
		if (USISessionSubsystem* Subsystem = GameInstanceRef->GetSubsystem<USISessionSubsystem>())
		{
			Subsystem->OnCreateSessionCompleteEvent.RemoveDynamic(
				this, &ASITitlePlayerController::HandleCreateSessionResult);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ASITitlePlayerController::HandleCreateSessionResult(const bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		return;
	}

	ShowNotice(FText::FromString(TEXT("방을 만들지 못했습니다. 잠시 후 다시 시도해주세요.")));
}

void ASITitlePlayerController::ShowNotice(const FText& Message)
{
	if (!IsValid(UIManagerComponent))
	{
		return;
	}

	USINoticeWidget* NoticeWidget = Cast<USINoticeWidget>(UIManagerComponent->OpenWidget(EUIType::Notice));

	if (!IsValid(NoticeWidget))
	{
		return;
	}

	NoticeWidget->SetMessage(Message);
}

void ASITitlePlayerController::ShowPendingFailureNotice()
{
	USISessionSubsystem* Subsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USISessionSubsystem>()
		: nullptr;

	if (!IsValid(Subsystem))
	{
		return;
	}

	ESIConnectionFailureType FailureType = ESIConnectionFailureType::Unknown;
	FString RawMessage;

	// 우편함이 비어 있으면 정상 진입 — 아무것도 하지 않는다
	if (!Subsystem->ConsumeLastFailure(FailureType, RawMessage))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Title] 직전 접속 실패: %s"), *RawMessage);

	ShowNotice(MakeFailureMessage(FailureType));
}

FText ASITitlePlayerController::MakeFailureMessage(const ESIConnectionFailureType FailureType)
{
	switch (FailureType)
	{
	case ESIConnectionFailureType::WrongPassword:
		return FText::FromString(TEXT("비밀번호가 일치하지 않습니다."));

	case ESIConnectionFailureType::ConnectionLost:
		return FText::FromString(TEXT("호스트가 방을 나갔거나 연결이 끊어졌습니다."));

	default:
		return FText::FromString(TEXT("방에 접속하지 못했습니다."));
	}
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
	USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>();

	if (!IsValid(SIInstance))
	{
		ShowNotice(FText::FromString(TEXT("방을 만들지 못했습니다.")));
		return;
	}

	SIInstance->CreateRoom(Settings);
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
