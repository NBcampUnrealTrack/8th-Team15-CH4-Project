// SIRoomListWidget.cpp


#include "UI/SIRoomListWidget.h"

#include "Components/Button.h"

void USIRoomListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &USIRoomListWidget::HandleBackClicked);
	}

	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &USIRoomListWidget::HandleCreateClicked);
	}
}

void USIRoomListWidget::HandleBackClicked()
{
	HandleCancelled();
}

void USIRoomListWidget::HandleCreateClicked()
{
	HandleConfirmed();
	
}