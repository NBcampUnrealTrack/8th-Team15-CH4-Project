// SINoticeWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SINoticeWidget.generated.h"

class UButton;
class UTextBlock;

/** 확인 버튼 하나짜리 단순 안내창. 접속 실패/연결 끊김 사유를 알리는 데 쓴다. */
UCLASS()
class TEAMPROJECT_API USINoticeWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	/** 표시할 문구. OpenWidget 직후에 호출한다. */
	void SetMessage(const FText& InMessage);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleConfirmClicked();

private:
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UButton> Button_Confirm;

	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Message;

	/** NativeConstruct 전에 SetMessage가 불릴 수 있어 보관해둔다 */
	FText PendingMessage;
};
