// SINameplateWidget.cpp


#include "UI/SINameplateWidget.h"
#include "Components/TextBlock.h"


void USINameplateWidget::SetPlayerName(const FString& PlayerName)
{
	if (!Text_PlayerName)
	{
		return;
	}

	Text_PlayerName->SetText(FText::FromString(PlayerName));
}
