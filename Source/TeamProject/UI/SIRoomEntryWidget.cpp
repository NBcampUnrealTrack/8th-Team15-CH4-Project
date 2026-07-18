// SIRoomEntryWidget.cpp


#include "UI/SIRoomEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void USIRoomEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Select)
	{
		Button_Select->OnClicked.AddDynamic(this, &USIRoomEntryWidget::HandleEntryClicked);
	}
}

void USIRoomEntryWidget::NativeDestruct()
{
	if (Button_Select)
	{
		Button_Select->OnClicked.RemoveDynamic(this, &USIRoomEntryWidget::HandleEntryClicked);
	}

	Super::NativeDestruct();
}

void USIRoomEntryWidget::Setup(const FSISessionInfo& InSessionInfo)
{
	SessionInfo = InSessionInfo;

	if (Text_RoomTitle)
	{
		Text_RoomTitle->SetText(FText::FromString(SessionInfo.RoomTitle));
	}

	if (Text_PlayerCount)
	{
		Text_PlayerCount->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), SessionInfo.CurrentPlayers, SessionInfo.MaxPlayers)));
	}

	if (Text_RoomState)
	{
		Text_RoomState->SetText(FText::FromString(
			SessionInfo.bIsInProgress ? TEXT("게임중") : TEXT("대기중")));
	}

	if (Icon_Lock)
	{
		Icon_Lock->SetVisibility(
			SessionInfo.bHasPassword ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void USIRoomEntryWidget::SetSelected(const bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}

	bSelected = bInSelected;
	OnSelectionChanged(bSelected);
}

void USIRoomEntryWidget::HandleEntryClicked()
{
	OnRoomEntrySelected.Broadcast(SessionInfo.SearchResultIndex);
}
