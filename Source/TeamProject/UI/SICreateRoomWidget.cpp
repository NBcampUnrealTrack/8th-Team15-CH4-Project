// CreateRoomWidget.cpp


#include "UI/SICreateRoomWidget.h"

#include "Components/CheckBox.h"
#include "Components/Button.h"

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
