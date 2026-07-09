// SICreateRoomWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SICreateRoomWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClickedBackButton);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClickedCreateButton);

class UCheckBox;
class UButton;

UCLASS()
class TEAMPROJECT_API USICreateRoomWidget : public USIUserWidget
{
	GENERATED_BODY()

private:
	virtual void NativeConstruct() override;

public:
    UPROPERTY()
	FOnClickedBackButton OnClickedBackButton;

	UPROPERTY()
	FOnClickedCreateButton OnClickedCreateButton;

private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleCreateClicked();


private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_Public;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_Private;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Create;

	bool bIsPrivate;

private:
	UFUNCTION()
	void HandlePublicChecked(bool bIsChecked);

	UFUNCTION()
	void HandlePrivateChecked(bool bIsChecked);
};
