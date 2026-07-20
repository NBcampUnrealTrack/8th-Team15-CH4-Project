// Fill out your copyright notice in the Description page of Project Settings.


#include "SIGameInstance.h"
#include "UI/SILodingWidget.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"	// SealMatchRoster에서 GetUniqueId() 사용
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

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

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &USIGameInstance::HandlePostLoadMap);
}

void USIGameInstance::Shutdown()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	Super::Shutdown();
}

void USIGameInstance::ShowLoadingScreen()
{
	// 띄울 위젯이 없으면 트래블을 건너 살려둘 이유도 없다.
	// 여기서 끊지 않으면 폴링이 매 틱 생성을 재시도하며 로그를 도배한다.
	if (!LoadingWidgetClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameInstance] 로딩 화면 생략: BP_GameInstance의 LoadingWidgetClass가 비어 있음"));
		return;
	}

	bLoadingScreenActive = true;
	LoadingScreenElapsedTime = 0.0f;

	CreateLoadingWidget();
}

void USIGameInstance::HideLoadingScreen()
{
	bLoadingScreenActive = false;
	LoadingScreenElapsedTime = 0.0f;

	ClearLoadingScreenTimer();
	RemoveLoadingWidget();
}

void USIGameInstance::RemoveLoadingWidget()
{
	if (IsValid(LoadingWidget))
	{
		LoadingWidget->RemoveFromParent();
	}

	LoadingWidget = nullptr;
}

void USIGameInstance::CreateLoadingWidget()
{
	if (IsValid(LoadingWidget) && LoadingWidget->IsInViewport())
	{
		return;
	}

	// 클래스 유효성은 ShowLoadingScreen에서 이미 걸렀다

	// 어느 뷰포트에 붙을지 정해주려면 로컬 PC가 필요하다.
	// 트래블 직후엔 아직 없을 수 있어 실패해도 그냥 넘어가고 폴링이 다시 시도한다.
	APlayerController* PC = GetFirstLocalPlayerController();
	if (!PC)
	{
		return;
	}

	LoadingWidget = CreateWidget<USILodingWidget>(PC, LoadingWidgetClass);
	if (!IsValid(LoadingWidget))
	{
		return;
	}

	LoadingWidget->AddToViewport(LoadingScreenZOrder);
}

void USIGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!bLoadingScreenActive || !IsValid(LoadedWorld))
	{
		return;
	}

	// PostLoadMapWithWorld는 전역 델리게이트라 PIE에선 다른 창의 맵 로드까지 여기로 들어온다.
	// 걸러내지 않으면 남의 로드에 반응해 내 뷰포트의 위젯을 놓치고 새로 만들어, 뗄 수 없는 위젯이 화면에 남는다.
	if (LoadedWorld->GetGameInstance() != this)
	{
		return;
	}

	// 이전 월드와 함께 사라졌을 위젯 — 살아 있다면 확실히 떼고 새로 만든다
	RemoveLoadingWidget();
	LoadingScreenElapsedTime = 0.0f;

	CreateLoadingWidget();

	LoadedWorld->GetTimerManager().SetTimer(
		LoadingScreenPollTimer, this, &USIGameInstance::PollLevelReady,
		LoadingScreenPollInterval, true);
}

void USIGameInstance::PollLevelReady()
{
	LoadingScreenElapsedTime += LoadingScreenPollInterval;

	// PC 생성이 늦었다면 이 시점에 위젯이 비어 있을 수 있다
	CreateLoadingWidget();

	const UWorld* World = GetWorld();
	const APlayerController* PC = GetFirstLocalPlayerController();

	// GameState까지 와야 로비/인게임 위젯들이 구독할 대상이 생긴다.
	// 그 전에 로딩 화면을 내리면 텅 빈 UI가 한 프레임 노출된다.
	const bool bLevelReady = IsValid(World) && PC && IsValid(World->GetGameState());

	if (bLevelReady || LoadingScreenElapsedTime >= MaxLoadingScreenSeconds)
	{
		if (!bLevelReady)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameInstance] 로딩 화면 타임아웃(%.0f초) — 준비 신호 없이 내림"), MaxLoadingScreenSeconds);
		}

		HideLoadingScreen();
	}
}

void USIGameInstance::ClearLoadingScreenTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoadingScreenPollTimer);
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

void USIGameInstance::SealMatchRoster(const TArray<APlayerState*>& Players)
{
	MatchRosterIds.Empty();

	for (const APlayerState* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		const FUniqueNetIdRepl& PlayerId = Player->GetUniqueId();
		if (PlayerId.IsValid())
		{
			MatchRosterIds.Add(PlayerId.ToString());
		}
	}

	bMatchRosterSealed = true;

	UE_LOG(LogTemp, Log, TEXT("[GameInstance] 매치 명단 봉인: %d명 (이후 접속은 중도 참여로 거절)"),
		MatchRosterIds.Num());
}

void USIGameInstance::ClearMatchRoster()
{
	MatchRosterIds.Empty();
	bMatchRosterSealed = false;
}

bool USIGameInstance::IsInMatchRoster(const FUniqueNetIdRepl& PlayerId) const
{
	return PlayerId.IsValid() && MatchRosterIds.Contains(PlayerId.ToString());
}

void USIGameInstance::ConsumeMatchRosterEntry(const FUniqueNetIdRepl& PlayerId)
{
	if (!bMatchRosterSealed || !PlayerId.IsValid())
	{
		return;
	}

	if (MatchRosterIds.Remove(PlayerId.ToString()) > 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[GameInstance] 매치 입장권 사용 — 남은 인원: %d"),
			MatchRosterIds.Num());
	}
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
