// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Blueprint/UserWidget.h"
#include "SIGameMode.generated.h"

/**
 * 
 */
UCLASS()
class TEAMPROJECT_API ASIGameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "MainMene UI")
	TSubclassOf<UUserWidget> MainMenuWidget;
};
