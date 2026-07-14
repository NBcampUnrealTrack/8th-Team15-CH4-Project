// SIPaletteColorEntry.h

#pragma once

#include "CoreMinimal.h"
#include "SIPaletteColorEntry.generated.h"


USTRUCT()
struct FSIPaletteColorEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	FLinearColor Color;
	
	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;
};