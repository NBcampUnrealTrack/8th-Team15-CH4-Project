// SIUserWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SIUserWidget.generated.h"

class UEditableText;
class UScrollBox;
class USIChatLineWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCancelled);

UCLASS()
class TEAMPROJECT_API USIUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnConfirmed OnConfirmed;
	FOnCancelled OnCancelled;

	/** 이 위젯의 채팅 입력칸에 키보드 포커스를 준다.
		PlayerController가 Enter를 받았을 때 "지금 화면에 있는 채팅창"을 대상으로 호출한다.
		채팅창이 없는 위젯은 기본 구현(아무것도 안 함)을 그대로 쓰면 된다. */
	virtual void FocusChatInput() {}

protected:
	void HandleConfirmed();
	void HandleCancelled();

	/** 입력칸이 숫자만 받도록 거른다. 대상의 OnTextChanged 핸들러에서 그대로 호출하면 된다.
		커밋이 아니라 입력 순간에 거르는 이유: 잘못 친 글자가 화면에 남아 있다가
		엔터를 쳐야 사라지면 "먹히는 줄 알았다"가 된다.

		★ 재귀 주의 — SetText는 내용이 실제로 바뀌면 OnTextChanged를 다시 쏜다
		  (SlateEditableTextLayout::SetText → OwnerWidget->OnTextChanged).
		  걸러낼 게 없으면 SetText를 부르지 않는 것으로 두 번째 호출에서 멈춘다.
		  이 가드가 한 곳에만 있도록 여기 베이스에 둔다. */
	static void FilterEditableTextToDigits(UEditableText* Target, const FText& Text);

	/** ── 채팅 그리기 (인게임 HUD / 로비 방 설정이 공유) ──
		그리기만 담당한다. 기록은 ASIPlayerController가 GameState 방송을 받아 남기므로
		여기서 저장하면 같은 줄이 두 번 쌓인다. */
	void AddChatLineTo(UScrollBox* ScrollBox, TSubclassOf<USIChatLineWidget> LineClass,
		const FString& SenderName, const FString& Message);

	/** GameInstance에 쌓인 기록을 그대로 다시 그린다 (위젯이 새로 만들어질 때 1회).
		세션이 유지되는 동안 레벨을 오가도 대화가 이어져 보이게 하는 지점. */
	void RestoreChatHistoryTo(UScrollBox* ScrollBox, TSubclassOf<USIChatLineWidget> LineClass);
};
