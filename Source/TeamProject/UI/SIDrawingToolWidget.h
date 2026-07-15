// SIDrawingToolWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SIDrawingToolWidget.generated.h"

class UButton;
class USIColorPalette;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSIOnColorSelected, int32, ColorIndex, FLinearColor, Color);

UCLASS()
class TEAMPROJECT_API USIDrawingToolWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	USIDrawingToolWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Palette")
	void SelectPaletteColor(int32 ColorIndex);

	// 색상 버튼이 선택한 팔레트 인덱스와 실제 색상을 게임플레이 코드로 전달한다.
	UPROPERTY(BlueprintAssignable, Category = "Palette")
	FSIOnColorSelected OnColorSelected;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Palette")
	TObjectPtr<USIColorPalette> ColorPalette;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color5;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color6;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color7;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color8;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color9;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Color10;

	UFUNCTION()
	void HandleColor1Clicked();
	UFUNCTION()
	void HandleColor2Clicked();
	UFUNCTION()
	void HandleColor3Clicked();
	UFUNCTION()
	void HandleColor4Clicked();
	UFUNCTION()
	void HandleColor5Clicked();
	UFUNCTION()
	void HandleColor6Clicked();
	UFUNCTION()
	void HandleColor7Clicked();
	UFUNCTION()
	void HandleColor8Clicked();
	UFUNCTION()
	void HandleColor9Clicked();
	UFUNCTION()
	void HandleColor10Clicked();
};
