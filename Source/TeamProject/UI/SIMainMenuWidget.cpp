// SIMainMenuWidget.cpp


#include "UI/SIMainMenuWidget.h"
#include "Components/Button.h"

void USIMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_CreateRoom)
	{
		Button_CreateRoom->OnClicked.AddDynamic(this, &USIMainMenuWidget::HandleCreateRoomClicked);
	}

	if (Button_JoinRoom)
	{
		Button_JoinRoom->OnClicked.AddDynamic(this, &USIMainMenuWidget::HandleJoinRoomClicked);
	}

	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddDynamic(this, &USIMainMenuWidget::HandleQuitClicked);
	}
}

void USIMainMenuWidget::HandleCreateRoomClicked()
{
	OnClickedCreateRoomButton.Broadcast();
}

void USIMainMenuWidget::HandleJoinRoomClicked()
{
	OnClickedJoinRoomButton.Broadcast();
}

void USIMainMenuWidget::HandleQuitClicked()
{
	OnClickedQuitButton.Broadcast();
}
