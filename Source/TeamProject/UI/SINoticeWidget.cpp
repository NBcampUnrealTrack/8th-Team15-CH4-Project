// SINoticeWidget.cpp


#include "UI/SINoticeWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void USINoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddDynamic(this, &USINoticeWidget::HandleConfirmClicked);
	}

	if (Text_Message)
	{
		Text_Message->SetText(PendingMessage);
	}
}

void USINoticeWidget::NativeDestruct()
{
	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.RemoveDynamic(this, &USINoticeWidget::HandleConfirmClicked);
	}

	Super::NativeDestruct();
}

void USINoticeWidget::SetMessage(const FText& InMessage)
{
	PendingMessage = InMessage;

	if (Text_Message)
	{
		Text_Message->SetText(PendingMessage);
	}
}

void USINoticeWidget::HandleConfirmClicked()
{
	// 안내창의 "확인"은 창을 닫는 것 외에 하는 일이 없다.
	// UI 매니저에서 스택을 pop하는 경로가 OnCancelled 쪽이라 이걸 쓴다.
	HandleCancelled();
}
