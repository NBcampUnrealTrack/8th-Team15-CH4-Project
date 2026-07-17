// SIUserWidget.cpp


#include "UI/SIUserWidget.h"

void USIUserWidget::HandleConfirmed()
{
	OnConfirmed.Broadcast();
}

void USIUserWidget::HandleCancelled()
{
	OnCancelled.Broadcast();
}