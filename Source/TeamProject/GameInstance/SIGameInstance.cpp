// Fill out your copyright notice in the Description page of Project Settings.


#include "SIGameInstance.h"

#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* MainMenuLevelPath = TEXT("/Game/Shape_It/Level/MainMenu");
}

void USIGameInstance::Init()
{
	Super::Init();

	// 방을 벗어나는 경로는 여러 갈래(직접 나가기 / 호스트 이탈 / 타임아웃)지만
	// 도착점은 하나뿐이라 GameInstance가 한 번만 구독해둔다. 레벨이 바뀌어도 살아남는다.
	if (USISessionSubsystem* SessionSubsystem = GetSubsystem<USISessionSubsystem>())
	{
		SessionSubsystem->OnSessionLeftEvent.AddUniqueDynamic(this, &USIGameInstance::HandleSessionLeft);
	}
}

void USIGameInstance::LeaveRoom()
{
	if (USISessionSubsystem* SessionSubsystem = GetSubsystem<USISessionSubsystem>())
	{
		SessionSubsystem->LeaveSession();
		return;
	}

	// 서브시스템이 없으면 방송해줄 주체도 없으니 직접 복귀
	UE_LOG(LogTemp, Error, TEXT("[GameInstance] LeaveRoom: SessionSubsystem 없음 — 메인메뉴로 직행"));
	HandleSessionLeft(ESISessionLeaveReason::UserRequested);
}

void USIGameInstance::HandleSessionLeft(const ESISessionLeaveReason Reason)
{
	ClearChatHistory();
	ConsumePendingMatch();

	UE_LOG(LogTemp, Log, TEXT("[GameInstance] 방을 나감 (사유: %s) — 메인메뉴로 복귀"),
		Reason == ESISessionLeaveReason::UserRequested ? TEXT("유저 요청") : TEXT("연결 끊김"));

	// OpenLevel이 기존 접속을 끊어준다 (클라이언트/호스트 공통)
	UGameplayStatics::OpenLevel(this, FName(MainMenuLevelPath));
}

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

void USIGameInstance::PrepareForJoin()
{
	ClearChatHistory();
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
