// SIColorPalette.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "Structs/SIPaletteColorEntry.h"

#include "SIColorPalette.generated.h"


UCLASS(BlueprintType)
class TEAMPROJECT_API USIColorPalette : public UDataAsset
{
	GENERATED_BODY()

public:
	// UI와 서버가 같은 인덱스로 색상을 찾도록 단일 팔레트를 제공한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	TArray<FSIPaletteColorEntry> Colors;

	UFUNCTION(BlueprintPure, Category = "Palette")
	bool TryGetColor(int32 ColorIndex, FLinearColor& OutColor) const;

	static const TCHAR* GetDefaultPaletteAssetPath();
	static USIColorPalette* LoadDefaultPalette();
};
