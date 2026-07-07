// UExitWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "ExitWidget.generated.h"

class UButton;

UCLASS()
class TEAMPROJECT_API UExitWidget : public USIUserWidget
{
	GENERATED_BODY()

private:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Quit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Continue;

private:
	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnContinueClicked();
};
