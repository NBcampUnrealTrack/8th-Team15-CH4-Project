// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "DetailPanelWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAxisValueChanged, EAxis::Type, Axis, float, Value);

class USlider;

UCLASS()
class TEAMPROJECT_API UDetailPanelWidget : public USIUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnAxisValueChanged OnLocationChanged;

	UPROPERTY(BlueprintAssignable)
	FOnAxisValueChanged OnRotationChanged;

	UPROPERTY(BlueprintAssignable)
	FOnAxisValueChanged OnScaleChanged;

	void SetRotationValues(const FRotator& Rotation);
	void SetScaleValues(const FVector& Scale);
	void SetTransformControlsEnabled(bool bEnabled);

protected:
	virtual void NativeConstruct() override;

private:
	// ========= Location ==========
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Location_X;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Location_Y;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Location_Z;

	// ========= Rotation ==========
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Rotation_X;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Rotation_Y;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Rotation_Z;

	// ========= Scale ==========
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Scale_X;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Scale_Y;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Scale_Z;

private:
	bool bIsSynchronizingValues = false;
	bool bAreTransformControlsEnabled = false;

	UFUNCTION()
	void OnLocationXChanged(float Value);

	UFUNCTION()
	void OnLocationYChanged(float Value);

	UFUNCTION()
	void OnLocationZChanged(float Value);

	UFUNCTION()
	void OnRotationXChanged(float Value);

	UFUNCTION()
	void OnRotationYChanged(float Value);

	UFUNCTION()
	void OnRotationZChanged(float Value);

	UFUNCTION()
	void OnScaleXChanged(float Value);

	UFUNCTION()
	void OnScaleYChanged(float Value);

	UFUNCTION()
	void OnScaleZChanged(float Value);
};
