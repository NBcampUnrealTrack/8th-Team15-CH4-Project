// SIUserWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SIUserWidget.generated.h"

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
};
