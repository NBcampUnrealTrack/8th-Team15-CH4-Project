// SICreateRoomWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "GameInstance/SISessionSubsystem.h"
#include "SICreateRoomWidget.generated.h"

class UButton;
class UEditableText;

UCLASS()
class TEAMPROJECT_API USICreateRoomWidget : public USIUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
private:
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_RoomName;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_RoomPassword;
	
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UEditableText> EditableText_BuildTimeLimit;
	
	UPROPERTY(BlueprintReadOnly, Meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UEditableText> EditableText_MaxPlacedShapeCount;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Create;

public:
	FSICreateSessionParams GetRoomSettings() const;
	
private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleCreateClicked();

	/** 비밀번호 칸은 숫자만 받는다 (실제 필터는 USIUserWidget::FilterEditableTextToDigits).
		로비의 방 설정 편집과 규칙을 맞춰야 한다 — 여기서 문자를 허용하면
		로비에서 그 칸을 건드리는 순간 조용히 지워진다. */
	UFUNCTION()
	void HandleRoomPasswordChanged(const FText& Text);

	UFUNCTION()
	void HandleMaxPlacedShapeCountChanged(const FText& Text);
};
