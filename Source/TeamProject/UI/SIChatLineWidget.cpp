//  SIChatLineWidget.cpp


#include "UI/SIChatLineWidget.h"
#include "Components/TextBlock.h"


void USIChatLineWidget::SetMessage(const FString& Sender, const FString& Message)
{
	if (!Text_ChatLine)
	{
		return;
	}
	
	Text_ChatLine->SetText(FText::Format(INVTEXT("{0} : {1}"), FText::FromString(Sender), FText::FromString(Message)));
}
