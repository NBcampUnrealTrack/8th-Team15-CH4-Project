// SIUserWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SIUserWidget.generated.h"

class UEditableText;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCancelled);

UCLASS()
class TEAMPROJECT_API USIUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnConfirmed OnConfirmed;
	FOnCancelled OnCancelled;

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
};
