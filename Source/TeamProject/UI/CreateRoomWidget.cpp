// CreateRoomWidget.cpp


#include "UI/CreateRoomWidget.h"

#include "Components/CheckBox.h"
#include "Components/Button.h"

void UCreateRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CheckBox_Public)
	{
		CheckBox_Public->OnCheckStateChanged.AddDynamic(this, &UCreateRoomWidget::HandlePublicChecked);
	}

	if (CheckBox_Private)
	{
		CheckBox_Private->OnCheckStateChanged.AddDynamic(this, &UCreateRoomWidget::HandlePrivateChecked);
	}

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &UCreateRoomWidget::HandleBackClicked);
	}

	if (Button_Create)
	{
		Button_Create->OnClicked.AddDynamic(this, &UCreateRoomWidget::HandleCreateClicked);
	}
}

void UCreateRoomWidget::HandleBackClicked()
{
	OnClickedBackButton.Broadcast();
}

void UCreateRoomWidget::HandleCreateClicked()
{
	OnClickedCreateButton.Broadcast();
}

void UCreateRoomWidget::HandlePublicChecked(bool bIsChecked)
{
	if (bIsChecked)
	{
		CheckBox_Private->SetIsChecked(false);
		bIsPrivate = false;
	}
}

void UCreateRoomWidget::HandlePrivateChecked(bool bIsChecked)
{
	if (bIsChecked)
	{
		CheckBox_Public->SetIsChecked(false);
		bIsPrivate = true;
	}
}
