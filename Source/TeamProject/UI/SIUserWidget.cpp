// SIUserWidget.cpp


#include "UI/SIUserWidget.h"
#include "Components/Slider.h"

void USIUserWidget::HandleConfirmed()
{
	OnConfirmed.Broadcast();
}

void USIUserWidget::HandleCancelled()
{
	OnCancelled.Broadcast();
}