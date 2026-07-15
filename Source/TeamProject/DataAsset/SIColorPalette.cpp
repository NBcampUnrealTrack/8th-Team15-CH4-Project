// SIColorPalette.cpp


#include "DataAsset/SIColorPalette.h"

namespace
{
	constexpr TCHAR DefaultPaletteAssetPath[] = TEXT("/Game/Shape_It/UI/DataAsset/DA_PaletteColor.DA_PaletteColor");
}

bool USIColorPalette::TryGetColor(int32 ColorIndex, FLinearColor& OutColor) const
{
	if (!Colors.IsValidIndex(ColorIndex))
	{
		return false;
	}

	OutColor = Colors[ColorIndex].Color;
	return true;
}

const TCHAR* USIColorPalette::GetDefaultPaletteAssetPath()
{
	return DefaultPaletteAssetPath;
}

USIColorPalette* USIColorPalette::LoadDefaultPalette()
{
	// 기본 에셋 경로는 한 곳에서만 관리해 UI와 서버의 팔레트가 어긋나지 않게 한다.
	return LoadObject<USIColorPalette>(nullptr, GetDefaultPaletteAssetPath());
}

