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
	TObjectPtr<UEditableText> EditableText_GuessTimeLimit;
	
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
};
