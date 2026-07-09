// MainMenuWidget.cpp


#include "UI/MainMenuWidget.h"
#include "Components/Button.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_CreateRoom)
	{
		Button_CreateRoom->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleCreateRoomClicked);
	}

	if (Button_JoinRoom)
	{
		Button_JoinRoom->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleJoinRoomClicked);
	}

	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuitClicked);
	}
}

void UMainMenuWidget::HandleCreateRoomClicked()
{
	OnClickedCreateRoomButton.Broadcast();
}

void UMainMenuWidget::HandleJoinRoomClicked()
{
	OnClickedJoinRoomButton.Broadcast();
}

void UMainMenuWidget::HandleQuitClicked()
{
	OnClickedQuitButton.Broadcast();
}

