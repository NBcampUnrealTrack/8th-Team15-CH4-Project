// SIDrawingToolWidget.cpp


#include "UI/SIDrawingToolWidget.h"

#include "Components/Button.h"
#include "DataAsset/SIColorPalette.h"
#include "UObject/ConstructorHelpers.h"

USIDrawingToolWidget::USIDrawingToolWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 패키징에서도 기본 팔레트가 포함되도록 위젯 CDO가 에셋을 참조한다.
	static ConstructorHelpers::FObjectFinder<USIColorPalette> PaletteAsset(
		USIColorPalette::GetDefaultPaletteAssetPath());
	if (PaletteAsset.Succeeded())
	{
		ColorPalette = PaletteAsset.Object;
	}
}

void USIDrawingToolWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ColorPalette)
	{
		ColorPalette = USIColorPalette::LoadDefaultPalette();
	}

	// 기존 WBP의 8개 버튼을 팔레트 인덱스 0~7에 연결한다.
	if (Button_Color1) Button_Color1->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor1Clicked);
	if (Button_Color2) Button_Color2->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor2Clicked);
	if (Button_Color3) Button_Color3->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor3Clicked);
	if (Button_Color4) Button_Color4->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor4Clicked);
	if (Button_Color5) Button_Color5->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor5Clicked);
	if (Button_Color6) Button_Color6->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor6Clicked);
	if (Button_Color7) Button_Color7->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor7Clicked);
	if (Button_Color8) Button_Color8->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor8Clicked);
	if (Button_Color9) Button_Color9->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor9Clicked);
	if (Button_Color10) Button_Color10->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleColor10Clicked);
}

void USIDrawingToolWidget::NativeDestruct()
{
	if (Button_Color1) Button_Color1->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor1Clicked);
	if (Button_Color2) Button_Color2->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor2Clicked);
	if (Button_Color3) Button_Color3->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor3Clicked);
	if (Button_Color4) Button_Color4->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor4Clicked);
	if (Button_Color5) Button_Color5->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor5Clicked);
	if (Button_Color6) Button_Color6->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor6Clicked);
	if (Button_Color7) Button_Color7->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor7Clicked);
	if (Button_Color8) Button_Color8->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor8Clicked);
	if (Button_Color9) Button_Color9->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor9Clicked);
	if (Button_Color10) Button_Color10->OnClicked.RemoveDynamic(this, &ThisClass::HandleColor10Clicked);

	Super::NativeDestruct();
}

void USIDrawingToolWidget::SelectPaletteColor(int32 ColorIndex)
{
	FLinearColor SelectedColor;
	if (!ColorPalette || !ColorPalette->TryGetColor(ColorIndex, SelectedColor))
	{
		return;
	}

	OnColorSelected.Broadcast(ColorIndex, SelectedColor);
}

void USIDrawingToolWidget::HandleColor1Clicked() { SelectPaletteColor(0); }
void USIDrawingToolWidget::HandleColor2Clicked() { SelectPaletteColor(1); }
void USIDrawingToolWidget::HandleColor3Clicked() { SelectPaletteColor(2); }
void USIDrawingToolWidget::HandleColor4Clicked() { SelectPaletteColor(3); }
void USIDrawingToolWidget::HandleColor5Clicked() { SelectPaletteColor(4); }
void USIDrawingToolWidget::HandleColor6Clicked() { SelectPaletteColor(5); }
void USIDrawingToolWidget::HandleColor7Clicked() { SelectPaletteColor(6); }
void USIDrawingToolWidget::HandleColor8Clicked() { SelectPaletteColor(7); }
void USIDrawingToolWidget::HandleColor9Clicked() { SelectPaletteColor(8); }
void USIDrawingToolWidget::HandleColor10Clicked() { SelectPaletteColor(9); }

