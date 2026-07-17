// Fill out your copyright notice in the Description page of Project Settings.


#include "SIGameInstance.h"

#include "Kismet/GameplayStatics.h"

void USIGameInstance::CreateRoom(const FSIRoomSettings& NewRoomSettings)
{
	ClearChatHistory();
	
	RoomSettings = NewRoomSettings;
	
	UGameplayStatics::OpenLevel(
		this,
		FName("/Game/Shape_It/Level/Test_Lobby"),
		true,
		TEXT("listen?game=/Script/TeamProject.SILobbyGameMode")
	);
}

void USIGameInstance::JoinRoom(const FString& Address)
{
	ClearChatHistory();
	
	RoomSettings = FSIRoomSettings();
	
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

void USIGameInstance::AddChatLog(const FString& SenderName, const FString& Message)
{
	FChatLogEntry& Entry = ChatHistory.AddDefaulted_GetRef();
	Entry.SenderName = SenderName;
	Entry.Message = Message;

	if (ChatHistory.Num() > MaxChatHistoryCount)
	{
		ChatHistory.RemoveAt(0, ChatHistory.Num() - MaxChatHistoryCount);
	}
}

void USIGameInstance::ClearChatHistory()
{
	ChatHistory.Reset();
}
