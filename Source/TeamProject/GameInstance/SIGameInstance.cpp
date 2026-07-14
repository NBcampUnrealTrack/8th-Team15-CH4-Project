// Fill out your copyright notice in the Description page of Project Settings.


#include "SIGameInstance.h"

#include "Kismet/GameplayStatics.h"

void USIGameInstance::CreateRoom()
{
	UGameplayStatics::OpenLevel(
		this,
		FName("/Game/Shape_It/Level/Test_Lobby"),
		true,
		TEXT("listen?game=/Script/TeamProject.SILobbyGameMode")
	);
}

void USIGameInstance::JoinRoom(const FString& Address)
{
	UGameplayStatics::OpenLevel(this, FName(*Address), false);
}

void USIGameInstance::PreparePendingMatch(const int32 InExpectedPlayerCount)
{
	ExpectedPlayerCount = FMath::Max(InExpectedPlayerCount, 1);
	bPendingMatchStart = true;
}

bool USIGameInstance::IsPendingMatchReady(const int32 ConnectedPlayerCount) const
{
	return bPendingMatchStart && ConnectedPlayerCount >= ExpectedPlayerCount;
}

void USIGameInstance::ConsumePendingMatch()
{
	bPendingMatchStart = false;
	ExpectedPlayerCount = 0;
}
