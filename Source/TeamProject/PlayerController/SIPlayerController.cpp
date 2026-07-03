// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/SIPlayerController.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "UI/SIUserWidget.h"

ASIPlayerController::ASIPlayerController()
{
}

void ASIPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 인스턴스가 없을 때, StaticClass만 존재한다면
	if (!DetailPanelWidgetInstance && DetailPanelWidget)
	{
		// StaticClass를 통해 Instance화
		DetailPanelWidgetInstance = CreateWidget<UDetailPanelWidget>(this, DetailPanelWidget);
	}
		
	// 인스턴스가 존재한다면
	if (DetailPanelWidgetInstance)
	{
		// 뷰포트에 노출
		DetailPanelWidgetInstance->AddToViewport();
	}
}

void ASIPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	DetailPanelWidget = CreateWidget<UDetailPanelWidget>(this, DetailPanelWidget);

	DetailPanelWidget->AddToViewport();
}
