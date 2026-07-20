// SIUserWidget.cpp


#include "UI/SIUserWidget.h"

#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "GameInstance/SIGameInstance.h"
#include "GameFramework/PlayerState.h"
#include "UI/SIChatLineWidget.h"

FString USIUserWidget::ResolveChatSenderName(const FChatMessagePayload& Payload)
{
	// 시스템 안내는 이름을 붙이지 않는다 ("왕건 : 왕건님이 입장했습니다"가 되지 않도록)
	if (Payload.bIsSystemMessage)
	{
		return FString();
	}

	return IsValid(Payload.Sender) ? Payload.Sender->GetPlayerName() : TEXT("???");
}

void USIUserWidget::AddChatLineTo(UScrollBox* ScrollBox, TSubclassOf<USIChatLineWidget> LineClass,
	const FString& SenderName, const FString& Message)
{
	if (!IsValid(ScrollBox) || !LineClass)
	{
		return;
	}

	USIChatLineWidget* Line = CreateWidget<USIChatLineWidget>(this, LineClass);
	if (!IsValid(Line))
	{
		return;
	}

	Line->SetMessage(SenderName, Message);

	ScrollBox->AddChild(Line);
	ScrollBox->ScrollToEnd();
}

void USIUserWidget::RestoreChatHistoryTo(UScrollBox* ScrollBox, TSubclassOf<USIChatLineWidget> LineClass)
{
	const USIGameInstance* GI = GetGameInstance<USIGameInstance>();
	if (!IsValid(GI))
	{
		return;
	}

	for (const FChatLogEntry& Entry : GI->GetChatHistory())
	{
		AddChatLineTo(ScrollBox, LineClass, Entry.SenderName, Entry.Message);
	}
}

void USIUserWidget::FilterEditableTextToDigits(UEditableText* Target, const FText& Text)
{
	if (!IsValid(Target))
	{
		return;
	}

	const FString Raw = Text.ToString();

	FString DigitsOnly;
	DigitsOnly.Reserve(Raw.Len());
	for (const TCHAR Character : Raw)
	{
		if (FChar::IsDigit(Character))
		{
			DigitsOnly.AppendChar(Character);
		}
	}

	// 걸러낼 게 없으면 여기서 끝 — SetText를 부르지 않으므로 재귀가 멈춘다 (헤더 주석 참고)
	if (DigitsOnly.Len() == Raw.Len())
	{
		return;
	}

	// 포커스 중 SetText는 커서를 맨 뒤로 보낸다(JumpTo(EndOfDocument)).
	// 문자열 중간에 문자를 끼워 넣으면 커서가 끝으로 튀지만,
	// 숫자만 받는 칸이라 실사용에 문제되지 않아 그대로 둔다.
	Target->SetText(FText::FromString(DigitsOnly));
}

void USIUserWidget::HandleConfirmed()
{
	OnConfirmed.Broadcast();
}

void USIUserWidget::HandleCancelled()
{
	OnCancelled.Broadcast();
}