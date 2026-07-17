// SICreateRoomWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "Enums/SITypes.h"
#include "SICreateRoomWidget.generated.h"

class UCheckBox;
class UButton;
class UEditableText;

UCLASS()
class TEAMPROJECT_API USICreateRoomWidget : public USIUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_Public;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_Private;
	
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

	bool bIsPrivate = false;
	
public:
	FSIRoomSettings GetRoomSettings() const;
	
private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleCreateClicked();
	
	UFUNCTION()
	void HandlePublicChecked(bool bIsChecked);

	UFUNCTION()
	void HandlePrivateChecked(bool bIsChecked);
};
