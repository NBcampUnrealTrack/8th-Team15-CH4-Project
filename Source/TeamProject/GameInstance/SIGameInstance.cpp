// Fill out your copyright notice in the Description page of Project Settings.


#include "SIGameInstance.h"

#include "Kismet/GameplayStatics.h"

void USIGameInstance::CreateRoom()
{
	UGameplayStatics::OpenLevel(
		this,
		FName("/Game/Maps/LobbyLevel"),
		true,
		TEXT("listen")
	);
}

void USIGameInstance::JoinRoom(const FString& Address)
{
	UGameplayStatics::OpenLevel(this, FName(*Address), false);
}
