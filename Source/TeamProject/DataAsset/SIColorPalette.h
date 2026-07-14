// SIColorPalette.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "Structs/SIPaletteColorEntry.h"

#include "SIColorPalette.generated.h"


UCLASS()
class TEAMPROJECT_API USIColorPalette : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	TArray<FSIPaletteColorEntry> Colors;
};
