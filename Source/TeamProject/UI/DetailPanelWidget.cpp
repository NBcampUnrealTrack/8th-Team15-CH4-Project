// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DetailPanelWidget.h"
#include "Components/Slider.h"

namespace
{
	constexpr float RotationRangeDegrees = 180.0f;
	constexpr float SliderMinValue = 0.0f;
	constexpr float SliderMaxValue = 2.0f;
	constexpr float SliderCenterValue = 1.0f;
	constexpr float MinScaleValue = 0.2f;
	constexpr float MaxScaleValue = 1.8f;

	//슬라이더의 최소값 / 최대값 설정
	void ConfigureSlider(USlider* Slider)
	{
		if (!Slider)
		{
			return;
		}

		Slider->SetMinValue(SliderMinValue);
		Slider->SetMaxValue(SliderMaxValue);
	}

	//슬라이더 값이 최소~최대 범위를 벗어나지 않게 제한
	float ClampSliderValue(float Value)
	{
		return FMath::Clamp(Value, SliderMinValue, SliderMaxValue);
	}

	//회전 각도를 슬라이더 값으로 변환
	float RotationDegreesToSliderValue(float Degrees)
	{
		return FMath::Clamp((Degrees / RotationRangeDegrees) + SliderCenterValue, SliderMinValue, SliderMaxValue);
	}

	//스케일 값을 슬라이더 값으로 변환
	float ScaleToSliderValue(float Scale)
	{
		return FMath::GetMappedRangeValueClamped(FVector2D(MinScaleValue, MaxScaleValue), FVector2D(SliderMinValue, SliderMaxValue), Scale);
	}
}

void UDetailPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ConfigureSlider(Location_X);
	if (Location_X)
	{
		Location_X->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnLocationXChanged);
	}
	ConfigureSlider(Location_Y);
	if (Location_Y)
	{
		Location_Y->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnLocationYChanged);
	}
	ConfigureSlider(Location_Z);
	if (Location_Z)
	{
		Location_Z->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnLocationZChanged);
	}

	ConfigureSlider(Rotation_X);
	if (Rotation_X)
	{
		Rotation_X->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnRotationXChanged);
	}
	ConfigureSlider(Rotation_Y);
	if (Rotation_Y)
	{
		Rotation_Y->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnRotationYChanged);
	}
	ConfigureSlider(Rotation_Z);
	if (Rotation_Z)
	{
		Rotation_Z->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnRotationZChanged);
	}

	ConfigureSlider(Scale_X);
	if (Scale_X)
	{
		Scale_X->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnScaleXChanged);
	}
	ConfigureSlider(Scale_Y);
	if (Scale_Y)
	{
		Scale_Y->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnScaleYChanged);
	}
	ConfigureSlider(Scale_Z);
	if (Scale_Z)
	{
		Scale_Z->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnScaleZChanged);
	}

	SetTransformControlsEnabled(false);
}

//일단 Location은 뺐습니다.

void UDetailPanelWidget::SetRotationValues(const FRotator& Rotation)
{
	bIsSynchronizingValues = true;

	if (Rotation_X)
	{
		Rotation_X->SetValue(RotationDegreesToSliderValue(Rotation.Roll));
	}
	if (Rotation_Y)
	{
		Rotation_Y->SetValue(RotationDegreesToSliderValue(Rotation.Pitch));
	}
	if (Rotation_Z)
	{
		Rotation_Z->SetValue(RotationDegreesToSliderValue(Rotation.Yaw));
	}

	bIsSynchronizingValues = false;
}

void UDetailPanelWidget::SetScaleValues(const FVector& Scale)
{
	bIsSynchronizingValues = true;

	if (Scale_X)
	{
		Scale_X->SetValue(ScaleToSliderValue(Scale.X));
	}
	if (Scale_Y)
	{
		Scale_Y->SetValue(ScaleToSliderValue(Scale.Y));
	}
	if (Scale_Z)
	{
		Scale_Z->SetValue(ScaleToSliderValue(Scale.Z));
	}

	bIsSynchronizingValues = false;
}

void UDetailPanelWidget::SetTransformControlsEnabled(bool bEnabled)
{
	bAreTransformControlsEnabled = bEnabled;

	if (!bEnabled)
	{
		SetRotationValues(FRotator::ZeroRotator);
		SetScaleValues(FVector::OneVector);
	}

	if (Location_X)
	{
		Location_X->SetIsEnabled(bEnabled);
	}
	if (Location_Y)
	{
		Location_Y->SetIsEnabled(bEnabled);
	}
	if (Location_Z)
	{
		Location_Z->SetIsEnabled(bEnabled);
	}
	if (Rotation_X)
	{
		Rotation_X->SetIsEnabled(bEnabled);
	}
	if (Rotation_Y)
	{
		Rotation_Y->SetIsEnabled(bEnabled);
	}
	if (Rotation_Z)
	{
		Rotation_Z->SetIsEnabled(bEnabled);
	}
	if (Scale_X)
	{
		Scale_X->SetIsEnabled(bEnabled);
	}
	if (Scale_Y)
	{
		Scale_Y->SetIsEnabled(bEnabled);
	}
	if (Scale_Z)
	{
		Scale_Z->SetIsEnabled(bEnabled);
	}
}

void UDetailPanelWidget::OnLocationXChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnLocationChanged.Broadcast(EAxis::X, Value);
}

void UDetailPanelWidget::OnLocationYChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnLocationChanged.Broadcast(EAxis::Y, Value);
}

void UDetailPanelWidget::OnLocationZChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnLocationChanged.Broadcast(EAxis::Z, Value);
}

void UDetailPanelWidget::OnRotationXChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnRotationChanged.Broadcast(EAxis::X, ClampSliderValue(Value));
}

void UDetailPanelWidget::OnRotationYChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnRotationChanged.Broadcast(EAxis::Y, ClampSliderValue(Value));
}

void UDetailPanelWidget::OnRotationZChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnRotationChanged.Broadcast(EAxis::Z, ClampSliderValue(Value));
}

void UDetailPanelWidget::OnScaleXChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnScaleChanged.Broadcast(EAxis::X, ClampSliderValue(Value));
}

void UDetailPanelWidget::OnScaleYChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnScaleChanged.Broadcast(EAxis::Y, ClampSliderValue(Value));
}

void UDetailPanelWidget::OnScaleZChanged(float Value)
{
	if (bIsSynchronizingValues || !bAreTransformControlsEnabled)
	{
		return;
	}

	OnScaleChanged.Broadcast(EAxis::Z, ClampSliderValue(Value));
}
