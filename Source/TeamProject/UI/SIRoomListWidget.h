// SIRoomListWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SIRoomListWidget.generated.h"

class UButton;

UCLASS()
class TEAMPROJECT_API USIRoomListWidget : public USIUserWidget
{
	GENERATED_BODY()
	
private:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleJoinClicked();
};
