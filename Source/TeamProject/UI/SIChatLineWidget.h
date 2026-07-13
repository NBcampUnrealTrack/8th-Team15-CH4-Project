// SIChatLineWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SIChatLineWidget.generated.h"


class UTextBlock;

UCLASS()
class TEAMPROJECT_API USIChatLineWidget : public USIUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION()
	void SetMessage(const FString& Sender, const FString& Message);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ChatLine;
};
