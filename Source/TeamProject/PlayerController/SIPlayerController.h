// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SIPlayerController.generated.h"

class UDetailPanelWidget;

UCLASS()
class TEAMPROJECT_API ASIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASIPlayerController();

private:
	virtual void ReceivedPlayer() override;
	
#pragma region UI
	
protected:
	// 액터 변형 관련 UI Widget Class
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UDetailPanelWidget> DetailPanelWidget;
	
	// 액터 변형 관련 UI Widget Instance
	UPROPERTY()
	TObjectPtr<UDetailPanelWidget> DetailPanelWidgetInstance;
	
public:
	UDetailPanelWidget* GetDetailPanelWidget() const { return DetailPanelWidgetInstance; };
	
#pragma endregion
	
};


