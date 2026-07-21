// CreateRoomWidget.cpp


#include "UI/SICreateRoomWidget.h"

#include "Components/Button.h"
#include "Components/EditableText.h"

void USICreateRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &USICreateRoomWidget::HandleBackClicked);
	}

	if (Button_Create)
	{
		Button_Create->OnClicked.AddDynamic(this, &USICreateRoomWidget::HandleCreateClicked);
	}

	if (EditableText_RoomPassword)
	{
		EditableText_RoomPassword->OnTextChanged.AddDynamic(
			this, &USICreateRoomWidget::HandleRoomPasswordChanged);
	}

	if (EditableText_MaxPlacedShapeCount)
	{
		EditableText_MaxPlacedShapeCount->OnTextChanged.AddDynamic(
			this, &USICreateRoomWidget::HandleMaxPlacedShapeCountChanged);
	}
}

void USICreateRoomWidget::HandleRoomPasswordChanged(const FText& Text)
{
	FilterEditableTextToDigits(EditableText_RoomPassword, Text);
}

void USICreateRoomWidget::HandleMaxPlacedShapeCountChanged(const FText& Text)
{
	FilterEditableTextToDigits(EditableText_MaxPlacedShapeCount, Text);
}

FSICreateSessionParams USICreateRoomWidget::GetRoomSettings() const
{
	FSICreateSessionParams Settings;

	const FString RoomTitle = EditableText_RoomName->GetText().ToString();
	const int32 BuildTime = FCString::Atoi(*EditableText_BuildTimeLimit->GetText().ToString());
	const int32 MaxPlacedShapeCount = EditableText_MaxPlacedShapeCount
		? FCString::Atoi(*EditableText_MaxPlacedShapeCount->GetText().ToString())
		: SIRoomSettingLimits::DefaultPlacedShapeCount;

	// 빈칸이면 대입하지 않아 구조체 기본값이 살아남는다.
	// 시간은 0 이하가 "미지정" 센티널 — GameMode가 자기 기본값을 쓴다.
	if (!RoomTitle.IsEmpty())
	{
		Settings.RoomTitle = RoomTitle;
	}

	// 비밀번호가 비어 있으면 그대로 공개방 — 별도의 공개/비공개 플래그는 두지 않는다
	Settings.Password = EditableText_RoomPassword->GetText().ToString();

	if (BuildTime > 0)
	{
		Settings.BuildTime = BuildTime;
	}

	Settings.MaxPlacedShapeCount = FMath::Clamp(
		MaxPlacedShapeCount > 0 ? MaxPlacedShapeCount : SIRoomSettingLimits::DefaultPlacedShapeCount,
		SIRoomSettingLimits::MinPlacedShapeCount,
		SIRoomSettingLimits::MaxPlacedShapeCount);

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
