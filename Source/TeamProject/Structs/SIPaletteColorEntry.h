// SIPaletteColorEntry.h

#pragma once

#include "CoreMinimal.h"
#include "SIPaletteColorEntry.generated.h"


USTRUCT(BlueprintType)
struct TEAMPROJECT_API FSIPaletteColorEntry
{
	GENERATED_BODY()

	// 팔레트 버튼과 도형 머티리얼에 공통으로 사용하는 색상이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FText DisplayName;
};
