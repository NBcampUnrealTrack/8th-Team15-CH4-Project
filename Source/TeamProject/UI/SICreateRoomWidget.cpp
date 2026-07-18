// CreateRoomWidget.cpp


#include "UI/SICreateRoomWidget.h"

#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/EditableText.h"

void USICreateRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CheckBox_Public)
	{
		CheckBox_Public->OnCheckStateChanged.AddDynamic(this, &USICreateRoomWidget::HandlePublicChecked);
	}

	if (CheckBox_Private)
	{
		CheckBox_Private->OnCheckStateChanged.AddDynamic(this, &USICreateRoomWidget::HandlePrivateChecked);
	}

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &USICreateRoomWidget::HandleBackClicked);
	}

	if (Button_Create)
	{
		Button_Create->OnClicked.AddDynamic(this, &USICreateRoomWidget::HandleCreateClicked);
	}
}

FSICreateSessionParams USICreateRoomWidget::GetRoomSettings() const
{
	FSICreateSessionParams Settings;

	const FString RoomTitle = EditableText_RoomName->GetText().ToString();
	const int32 BuildTime = FCString::Atoi(*EditableText_BuildTimeLimit->GetText().ToString());
	const int32 GuessTime = FCString::Atoi(*EditableText_GuessTimeLimit->GetText().ToString());

	// 빈칸이면 대입하지 않아 구조체 기본값이 살아남는다.
	// 시간은 0 이하가 "미지정" 센티널 — GameMode가 자기 기본값을 쓴다.
	if (!RoomTitle.IsEmpty())
	{
		Settings.RoomTitle = RoomTitle;
	}

	Settings.Password = EditableText_RoomPassword->GetText().ToString();
	Settings.bIsPrivate = bIsPrivate;

	if (BuildTime > 0)
	{
		Settings.BuildTime = BuildTime;
	}

	if (GuessTime > 0)
	{
		Settings.GuessTime = GuessTime;
	}

	return Settings;
}

void USICreateRoomWidget::HandleBackClicked()
{
	HandleCancelled();
}

void USICreateRoomWidget::HandleCreateClicked()
{
	HandleConfirmed();
}

void USICreateRoomWidget::HandlePublicChecked(bool bIsChecked)
{
	if (bIsChecked)
	{
		CheckBox_Private->SetIsChecked(false);
		bIsPrivate = false;
	}
}

void USICreateRoomWidget::HandlePrivateChecked(bool bIsChecked)
{
	if (bIsChecked)
	{
		CheckBox_Public->SetIsChecked(false);
		bIsPrivate = true;
	}
}
