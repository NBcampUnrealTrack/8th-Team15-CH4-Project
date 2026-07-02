// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DetailPanelWidget.h"
#include "Components/Slider.h"

void UDetailPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Location_X)
	{
		Location_X->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnLocationXChanged);
	}
	if (Location_Y)
	{
		Location_Y->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnLocationYChanged);
	}
	if (Location_Z)
	{
		Location_Z->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnLocationZChanged);
	}

	if (Rotation_X)
	{
		Rotation_X->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnRotationXChanged);
	}
	if (Rotation_Y)
	{
		Rotation_Y->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnRotationYChanged);
	}
	if (Rotation_Z)
	{
		Rotation_Z->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnRotationZChanged);
	}

	if (Scale_X)
	{
		Scale_X->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnScaleXChanged);
	}
	if (Scale_Y)
	{
		Scale_Y->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnScaleYChanged);
	}
	if (Scale_Z)
	{
		Scale_Z->OnValueChanged.AddDynamic(this, &UDetailPanelWidget::OnScaleZChanged);
	}

}

void UDetailPanelWidget::OnLocationXChanged(float Value)
{
	OnLocationChanged.Broadcast(EAxis::X, Value);
}

void UDetailPanelWidget::OnLocationYChanged(float Value)
{
	OnLocationChanged.Broadcast(EAxis::Y, Value);
}

void UDetailPanelWidget::OnLocationZChanged(float Value)
{
	OnLocationChanged.Broadcast(EAxis::Z, Value);
}

void UDetailPanelWidget::OnRotationXChanged(float Value)
{
	OnRotationChanged.Broadcast(EAxis::X, Value);
}

void UDetailPanelWidget::OnRotationYChanged(float Value)
{
	OnRotationChanged.Broadcast(EAxis::Y, Value);
}

void UDetailPanelWidget::OnRotationZChanged(float Value)
{
	OnRotationChanged.Broadcast(EAxis::Z, Value);
}

void UDetailPanelWidget::OnScaleXChanged(float Value)
{
	OnScaleChanged.Broadcast(EAxis::X, Value);
}

void UDetailPanelWidget::OnScaleYChanged(float Value)
{
	OnScaleChanged.Broadcast(EAxis::Y, Value);
}

void UDetailPanelWidget::OnScaleZChanged(float Value)
{
	OnScaleChanged.Broadcast(EAxis::Z, Value);
}