// Fill out your copyright notice in the Description page of Project Settings.


#include "SISessionSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

// 커스텀 세션 데이터 키 — 오타 방지를 위해 상수로 한 곳에
static const FName KEY_ROOM_TITLE(TEXT("SI_ROOM_TITLE"));
static const FName KEY_IS_PRIVATE(TEXT("SI_IS_PRIVATE"));

#pragma region LOG

/** 화면+로그 동시 출력 헬퍼 (멀티프로세스 테스트 필수 계측) */
static void PrintNS(const FString& Msg, const FColor Color = FColor::Cyan)
{
	UE_LOG(LogTemp, Warning, TEXT("[NSSession] %s"), *Msg);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 12.f, Color, FString::Printf(TEXT("[NSSession] %s"), *Msg));
	}
}

#pragma endregion

#pragma region UGameInstanceSubsystem Override

void USISessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	PrintNS(TEXT("Subsystem Initialized"));
	
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &USISessionSubsystem::HandleNetworkFailure);
	}
}

void USISessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);   // 구독-해제 짝 규칙, 여기서도
	}
	
	// GameInstance 종료 시 좀비 세션 방지 — 이전 프로젝트의 Shutdown() 역할이 여기로 이사
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		PrintNS(TEXT("Deinitialize: Destroying leftover session"));
		SessionInterface->DestroySession(NAME_GameSession);
	}
	
	Super::Deinitialize();
}

#pragma endregion

#pragma region Get SessionInterface

bool USISessionSubsystem::EnsureSessionInterface()
{
	// 이미 SessionInterface 사용 가능하다면 true 반환
	if (SessionInterface.IsValid())
	{
		return true;
	}

	// 현재 World를 기준으로 Subsystem을 가져온 후, Session Interface를 가져온다.
	if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		SessionInterface = Subsystem->GetSessionInterface();
		// SessionInterface 사용 가능하다면 true 반환
		if (SessionInterface.IsValid())
		{
			PrintNS(FString::Printf(TEXT("Found Subsystem: %s"), *Subsystem->GetSubsystemName().ToString()));
			return true;
		}
	}

	PrintNS(TEXT("ERROR: SessionInterface unavailable"), FColor::Red);
	return false;
}

#pragma endregion

#pragma region Create Session

void USISessionSubsystem::CreateSession(const FSICreateSessionParams& Params)
{
	// EnsureSessionInterface return value가 false라면 
	if (!EnsureSessionInterface())
	{
		// OnCreateSessionCompleteEvent를 false로 호출
		OnCreateSessionCompleteEvent.Broadcast(false);
		return;
	}

	// 방 설정 전체 보관 (Password 포함 — PreLogin 검증용)
	PendingParams = Params;

	// 기존 세션이 있으면: 파괴를 요청하고, 생성은 완료 콜백이 이어받는다 (비동기 체인)
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		PrintNS(TEXT("Existing session found. Destroy -> Create chain..."));
		bCreateSessionOnDestroy = true;
		DestroySession();
		return;
	}

	CreateSessionInternal();
}

void USISessionSubsystem::CreateSessionInternal()
{
	// Session Settings
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = true;   // Null = LAN. Steam 전환 시 서브시스템 이름 검사로 분기
	SessionSettings.NumPublicConnections = PendingParams.MaxPlayers;
	SessionSettings.bShouldAdvertise = true;	// 비공개 방도 목록엔 보여야 하므로 항상 true
	SessionSettings.bUsesPresence = true;	// Null에선 무의미하지만 마이그레이션 대비
	SessionSettings.bAllowJoinInProgress = true;
	
	// ── 커스텀 데이터 광고: 검색자가 목록에서 볼 정보만. Password는 절대 넣지 않는다 ──
	SessionSettings.Set(KEY_ROOM_TITLE, PendingParams.RoomTitle,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(KEY_IS_PRIVATE, PendingParams.bIsPrivate ? 1 : 0,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteDelegateHandle =
		SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(this, &USISessionSubsystem::OnCreateSessionComplete));

	PrintNS(TEXT("Creating session..."));
	if (!SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
	{
		// 요청 자체가 즉시 거부된 경우 (콜백이 안 올 수 있으므로 여기서 정리+방송)
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		PrintNS(TEXT("CreateSession request REJECTED"), FColor::Red);
		OnCreateSessionCompleteEvent.Broadcast(false);
	}
}

void USISessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	PrintNS(FString::Printf(TEXT("Create Complete: %s"), bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAIL")),
		bWasSuccessful ? FColor::Green : FColor::Red);

	// 서브시스템은 여기까지. 어느 맵을 listen으로 열지는 구독자(게임 쪽)의 결정
	OnCreateSessionCompleteEvent.Broadcast(bWasSuccessful);
}

#pragma endregion

#pragma region Find Session

void USISessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!EnsureSessionInterface())
	{
		OnFindSessionsCompleteEvent.Broadcast({}, false);
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = MaxSearchResults;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	FindSessionsCompleteDelegateHandle =
		SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
			FOnFindSessionsCompleteDelegate::CreateUObject(this, &USISessionSubsystem::OnFindSessionsComplete));

	PrintNS(TEXT("Searching for sessions..."));
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		PrintNS(TEXT("FindSessions request REJECTED"), FColor::Red);
		OnFindSessionsCompleteEvent.Broadcast({}, false);
	}
}

void USISessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

	// 원본(FOnlineSessionSearchResult) → BP 친화 구조체(FNSSessionInfo) 번역
	TArray<FSISessionInfo> Sessions;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& R = SessionSearch->SearchResults[i];

			FSISessionInfo Info;
			Info.SearchResultIndex = i;
			Info.HostName = R.Session.OwningUserName;
			Info.PingMs = R.PingInMs;

			// 커스텀 데이터 추출 — 호스트가 광고한 값 꺼내기
			R.Session.SessionSettings.Get(KEY_ROOM_TITLE, Info.RoomTitle);
			int32 IsPrivateInt = 0;
			R.Session.SessionSettings.Get(KEY_IS_PRIVATE, IsPrivateInt);
			Info.bIsPrivate = (IsPrivateInt != 0);

			// 인원: 최대 - 남은 슬롯 = 현재 인원
			Info.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
			Info.CurrentPlayers = Info.MaxPlayers - R.Session.NumOpenPublicConnections;

			Sessions.Add(Info);
		}
	}

	PrintNS(FString::Printf(TEXT("Find Complete: %s / Results: %d"),
		bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAIL"), Sessions.Num()));

	OnFindSessionsCompleteEvent.Broadcast(Sessions, bWasSuccessful);
}

#pragma endregion

#pragma region JoinSession

void USISessionSubsystem::JoinSessionByIndex(int32 SearchResultIndex, const FString& Password)
{
	if (!EnsureSessionInterface())
	{
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		PrintNS(FString::Printf(TEXT("Join FAIL: invalid index %d"), SearchResultIndex), FColor::Red);
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	JoinSessionCompleteDelegateHandle =
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &USISessionSubsystem::OnJoinSessionComplete));
	
	PendingJoinPassword = Password;   // private 멤버 FString 하나 추가해서 보관

	PrintNS(FString::Printf(TEXT("Joining session [%d]..."), SearchResultIndex));
	if (!SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SearchResultIndex]))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		PrintNS(TEXT("JoinSession request REJECTED"), FColor::Red);
		OnJoinSessionCompleteEvent.Broadcast(false);
	}
}

void USISessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		PrintNS(FString::Printf(TEXT("Join Complete: FAIL (%d)"), static_cast<int32>(Result)), FColor::Red);

		// 실패 시 로컬에 등록된 세션 정리 → 재시도 가능하게 ("can't join twice" 방지)
		if (SessionInterface->GetNamedSession(NAME_GameSession))
		{
			PrintNS(TEXT("Cleaning up local session after failed join"));
			DestroySession();
		}
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	if (!PC)
	{
		PrintNS(TEXT("Travel FAIL: PlayerController null"), FColor::Red);
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		PrintNS(TEXT("Travel FAIL: could not resolve connect string"), FColor::Red);
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}
	
	// 비밀번호가 있으면 URL 옵션으로 동봉 → 서버의 PreLogin이 읽는다
	if (!PendingJoinPassword.IsEmpty())
	{
		ConnectString += FString::Printf(TEXT("?Password=%s"), *PendingJoinPassword);
	}

	PrintNS(FString::Printf(TEXT("Traveling to: %s"), *ConnectString), FColor::Green);
	OnJoinSessionCompleteEvent.Broadcast(true);
	PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}

#pragma endregion

#pragma region Destory Session

void USISessionSubsystem::DestroySession()
{
	if (!EnsureSessionInterface())
	{
		OnDestroySessionCompleteEvent.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle =
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &USISessionSubsystem::OnDestroySessionComplete));

	PrintNS(TEXT("Destroying session..."));
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		PrintNS(TEXT("DestroySession request REJECTED"), FColor::Red);
		bCreateSessionOnDestroy = false;
		OnDestroySessionCompleteEvent.Broadcast(false);
	}
}

void USISessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	PrintNS(FString::Printf(TEXT("Destroy Complete: %s"), bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAIL")));

	OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);

	// Destroy → Create 체인의 연결 고리
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSessionInternal();
	}
}

#pragma endregion

#pragma region NetworkState

void USISessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	PrintNS(FString::Printf(TEXT("Network Failure: %s"), *ErrorString), FColor::Red);

	// 실패한 접속이 남긴 로컬 세션 정리 → 재시도 가능 상태 복구
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		DestroySession();
	}

	OnNetworkFailureEvent.Broadcast(false); 
}

#pragma endregion








