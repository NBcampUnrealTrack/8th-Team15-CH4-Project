// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SIGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TEAMPROJECT_API USIGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	void CreateRoom();   // 지금은 내부에서 OpenLevel(...,"listen") 호출
	void JoinRoom(const FString& Address); // 지금은 내부에서 OpenLevel(Address) 호출
};
