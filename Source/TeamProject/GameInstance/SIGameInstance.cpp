// Fill out your copyright notice in the Description page of Project Settings.


#include "SIGameInstance.h"

#include "Kismet/GameplayStatics.h"

void USIGameInstance::CreateRoom(const FSICreateSessionParams& NewRoomSettings)
{
	ClearChatHistory();

	USISessionSubsystem* SessionSubsystem = GetSubsystem<USISessionSubsystem>();

	if (!IsValid(SessionSubsystem))
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] CreateRoom 실패: SessionSubsystem 없음"));
		return;
	}

	// 설정은 서브시스템이 보관한다(PendingParams). 맵을 여는 건 생성 완료 뒤.
	SessionSubsystem->OnCreateSessionCompleteEvent.AddUniqueDynamic(
		this, &USIGameInstance::HandleCreateSessionComplete);

	SessionSubsystem->CreateSession(NewRoomSettings);
}

void USIGameInstance::HandleCreateSessionComplete(const bool bWasSuccessful)
{
	if (USISessionSubsystem* SessionSubsystem = GetSubsystem<USISessionSubsystem>())
	{
		SessionSubsystem->OnCreateSessionCompleteEvent.RemoveDynamic(
			this, &USIGameInstance::HandleCreateSessionComplete);
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] 세션 생성 실패 — 로비로 이동하지 않음"));
		return;
	}

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
