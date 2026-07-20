//  SIChatLineWidget.cpp


#include "UI/SIChatLineWidget.h"
#include "Components/TextBlock.h"


void USIChatLineWidget::SetMessage(const FString& Sender, const FString& Message)
{
	if (!Text_ChatLine)
	{
		return;
	}
	
	// 보낸 사람이 없으면 시스템 안내(입장/퇴장 등) — 본문만 그린다.
	// 저장된 기록에서 복원할 때도 같은 규칙이 적용되도록 이름 유무로 판단한다.
	if (Sender.IsEmpty())
	{
		Text_ChatLine->SetText(FText::FromString(Message));
		return;
	}

	Text_ChatLine->SetText(FText::Format(INVTEXT("{0} : {1}"), FText::FromString(Sender), FText::FromString(Message)));
}
