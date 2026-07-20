// Fill out your copyright notice in the Description page of Project Settings.


#include "SISessionSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystemNames.h"	// NULL_SUBSYSTEM
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

#include "Misc/Base64.h"	// 한글 방 제목 우회

#include "Engine/NetDriver.h"
#include "Engine/World.h"

// 커스텀 세션 데이터 키 — 오타 방지를 위해 상수로 한 곳에
static const FName KEY_ROOM_TITLE(TEXT("SI_ROOM_TITLE"));
static const FName KEY_HAS_PASSWORD(TEXT("SI_HAS_PASSWORD"));
static const FName KEY_IN_PROGRESS(TEXT("SI_IN_PROGRESS"));

/** ── 한글 방 제목 우회 (UE 5.5 OnlineSubsystemSteam 인코딩 버그) ──
	스팀 로비 데이터의 왕복 인코딩이 어긋나 있다.
	  쓰기: SetLobbyData(..., TCHAR_TO_UTF8(*Value))        → UTF-8
	  읽기: SteamKeyToSessionSetting의 case 's'에서
	        Setting.Data.SetValue(ANSI_TO_TCHAR(SteamValue)) → ANSI
	그래서 ASCII 밖의 문자(한글 등)는 검색하는 쪽에서 깨진다.
	(바로 옆 OwningUserName은 UTF8_TO_TCHAR를 제대로 써서 스팀 닉네임 한글은 멀쩡하다.)

	해결: 광고할 땐 Base64로 감싸 ASCII로만 만들고, 읽을 때 되돌린다.
	Base64 결과는 A-Z a-z 0-9 + / = 뿐이라 ANSI로 읽혀도 무손실이다.
	두 함수는 반드시 짝으로 쓸 것. */
static FString EncodeRoomTitleForAdvertising(const FString& RawTitle)
{
	return FBase64::Encode(RawTitle);
}

static FString DecodeAdvertisedRoomTitle(const FString& AdvertisedTitle)
{
	FString Decoded;
	if (FBase64::Decode(AdvertisedTitle, Decoded))
	{
		return Decoded;
	}

	// 디코드 실패 = Base64가 아님. 구버전 클라이언트가 만든 방이거나
	// 엔진이 나중에 고쳐져 원문이 그대로 오는 경우 → 있는 그대로 보여준다.
	return AdvertisedTitle;
}

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
	
	// PrintNS(TEXT("Subsystem Initialized"));
	
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &USISessionSubsystem::HandleNetworkFailure);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USISessionSubsystem::HandlePostLoadMap);
}

void USISessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// 내 GameInstance의 맵이 올라왔다 = 입장 시도가 끝났다(성공했든 되돌아왔든)
	if (LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		bJoinInProgress = false;
	}
}

void USISessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);   // 구독-해제 짝 규칙, 여기서도
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	
	// GameInstance 종료 시 좀비 세션 방지 — 이전 프로젝트의 Shutdown() 역할이 여기로 이사
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		// PrintNS(TEXT("Deinitialize: Destroying leftover session"));
		SessionInterface->DestroySession(NAME_GameSession);
	}

	// 참조를 놓지 않으면 FOnlineSubsystemNull::Shutdown의 IsUnique() ensure에 걸린다.
	// (PIE 종료 때마다 "Ensure condition failed: SessionInterface.IsUnique()")
	SessionSearch.Reset();
	SessionInterface.Reset();

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
			// PrintNS(FString::Printf(TEXT("Found Subsystem: %s"), *Subsystem->GetSubsystemName().ToString()));
			return true;
		}
	}

	// PrintNS(TEXT("ERROR: SessionInterface unavailable"), FColor::Red);
	return false;
}

bool USISessionSubsystem::IsLanOnlySubsystem() const
{
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		// 서브시스템조차 못 잡은 상황. LAN으로 취급하는 게 안전하다
		// (Steam 로비를 못 여는 것보다, 최소한 같은 망에서라도 붙는 편이 낫다)
		return true;
	}

	// Null 외의 구현체(Steam 등)는 전부 "인터넷 너머로 광고 가능"으로 본다.
	// 이름을 STEAM으로 화이트리스트하지 않고 NULL만 블랙리스트하는 이유:
	// 나중에 EOS 등으로 갈아타도 이 함수를 고칠 일이 없다.
	return Subsystem->GetSubsystemName() == NULL_SUBSYSTEM;
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
		// PrintNS(TEXT("Existing session found. Destroy -> Create chain..."));
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
	SessionSettings.bIsLANMatch = IsLanOnlySubsystem();   // Null = LAN 브로드캐스트 / Steam = 로비 서버
	SessionSettings.NumPublicConnections = PendingParams.MaxPlayers;
	SessionSettings.bShouldAdvertise = true;	// 모든 방은 목록에 노출된다. 입장 제한은 비밀번호로만 건다
	SessionSettings.bAllowJoinInProgress = true;

	// Steam 구현체는 이 둘을 "같은 의미"로 보고, 값이 어긋나면 경고를 뱉으며 동작이 갈린다.
	// (OnlineSessionInterfaceSteam.cpp: "treated as equal and have to match")
	// 켜져 있어야 전용 서버 없이 스팀 로비로 방이 열린다 — 우리 리슨 서버 구조의 전제.
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	
	// ── 커스텀 데이터 광고: 검색자가 목록에서 볼 정보만. Password는 절대 넣지 않는다 ──
	SessionSettings.Set(KEY_ROOM_TITLE, EncodeRoomTitleForAdvertising(PendingParams.RoomTitle),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(KEY_HAS_PASSWORD, PendingParams.Password.IsEmpty() ? 0 : 1,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(KEY_IN_PROGRESS, 0,	// 갓 만든 방은 항상 대기중
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteDelegateHandle =
		SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(this, &USISessionSubsystem::OnCreateSessionComplete));

	// PrintNS(TEXT("Creating session..."));
	if (!SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
	{
		// 요청 자체가 즉시 거부된 경우 (콜백이 안 올 수 있으므로 여기서 정리+방송)
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		// PrintNS(TEXT("CreateSession request REJECTED"), FColor::Red);
		OnCreateSessionCompleteEvent.Broadcast(false);
	}
}

void USISessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	// PrintNS(FString::Printf(TEXT("Create Complete: %s"), bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAIL")),
	// 	bWasSuccessful ? FColor::Green : FColor::Red);

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
	SessionSearch->bIsLanQuery = IsLanOnlySubsystem();   // 광고 방식과 반드시 짝이 맞아야 한다
	SessionSearch->MaxSearchResults = MaxSearchResults;
	// Steam은 이 플래그를 보고 서버 브라우저가 아닌 "로비 검색" 경로를 탄다.
	// (OnlineSessionInterfaceSteam.cpp의 SEARCH_PRESENCE/SEARCH_LOBBIES 분기 → FindLobbies)
	// Null에선 무시되므로 그대로 둬도 무해하다.
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	FindSessionsCompleteDelegateHandle =
		SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
			FOnFindSessionsCompleteDelegate::CreateUObject(this, &USISessionSubsystem::OnFindSessionsComplete));

	// PrintNS(TEXT("Searching for sessions..."));
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		// PrintNS(TEXT("FindSessions request REJECTED"), FColor::Red);
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
			FString AdvertisedTitle;
			R.Session.SessionSettings.Get(KEY_ROOM_TITLE, AdvertisedTitle);
			Info.RoomTitle = DecodeAdvertisedRoomTitle(AdvertisedTitle);
			int32 HasPasswordInt = 0;
			R.Session.SessionSettings.Get(KEY_HAS_PASSWORD, HasPasswordInt );
			Info.bHasPassword = (HasPasswordInt  != 0);

			int32 InProgressInt = 0;
			R.Session.SessionSettings.Get(KEY_IN_PROGRESS, InProgressInt);
			Info.bIsInProgress = (InProgressInt != 0);

			// 인원: 최대 - 남은 슬롯 = 현재 인원
			Info.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
			Info.CurrentPlayers = Info.MaxPlayers - R.Session.NumOpenPublicConnections;

			Sessions.Add(Info);
		}
	}

	// PrintNS(FString::Printf(TEXT("Find Complete: %s / Results: %d"),
	// 	bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAIL"), Sessions.Num()));

	OnFindSessionsCompleteEvent.Broadcast(Sessions, bWasSuccessful);
}

void USISessionSubsystem::UpdateHostSessionParams(const FSICreateSessionParams& NewParams)
{
	// 보관값은 항상 갱신한다 — PreLogin 비번 검증과 인게임 시간 적용이 이걸 읽는다
	PendingParams = NewParams;

	if (!EnsureSessionInterface())
	{
		return;
	}

	// 광고 갱신은 세션이 실제로 있을 때만 (호스트 아니면 원본이 없다)
	FOnlineSessionSettings* CurrentSettings = SessionInterface->GetSessionSettings(NAME_GameSession);
	if (!CurrentSettings)
	{
		return;
	}

	CurrentSettings->Set(KEY_ROOM_TITLE, EncodeRoomTitleForAdvertising(PendingParams.RoomTitle),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	CurrentSettings->Set(KEY_HAS_PASSWORD, PendingParams.Password.IsEmpty() ? 0 : 1,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// PrintNS(FString::Printf(TEXT("Room settings updated: %s"), *PendingParams.RoomTitle));

	SessionInterface->UpdateSession(NAME_GameSession, *CurrentSettings, true);
}

void USISessionSubsystem::SetSessionInProgress(const bool bInProgress)
{
	if (!EnsureSessionInterface())
	{
		return;
	}

	// 호스트만 세션 설정의 원본을 들고 있다. 참가자는 여기서 조용히 빠져나간다.
	FOnlineSessionSettings* CurrentSettings = SessionInterface->GetSessionSettings(NAME_GameSession);
	if (!CurrentSettings)
	{
		return;
	}

	int32 StoredValue = 0;
	CurrentSettings->Get(KEY_IN_PROGRESS, StoredValue);

	// 값이 그대로면 갱신 요청을 아낀다 (LAN 비콘 재방송 비용)
	if ((StoredValue != 0) == bInProgress)
	{
		return;
	}

	CurrentSettings->Set(KEY_IN_PROGRESS, bInProgress ? 1 : 0,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// PrintNS(FString::Printf(TEXT("Session state -> %s"), bInProgress ? TEXT("IN PROGRESS") : TEXT("WAITING")));

	SessionInterface->UpdateSession(NAME_GameSession, *CurrentSettings, true);
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
		// PrintNS(FString::Printf(TEXT("Join FAIL: invalid index %d"), SearchResultIndex), FColor::Red);
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	JoinSessionCompleteDelegateHandle =
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &USISessionSubsystem::OnJoinSessionComplete));
	
	PendingJoinPassword = Password;   // private 멤버 FString 하나 추가해서 보관
	bJoinInProgress = true;           // 이제부터 오는 접속 거절은 "내 것"이다

	FOnlineSessionSearchResult& DesiredSession = SessionSearch->SearchResults[SearchResultIndex];

	// ── 엔진 버그 우회 (UE 5.5 OnlineSubsystemSteam) ──
	// FOnlineSessionSteam::JoinSession은 맨 앞에서 "검색 결과"의 bUsesPresence와
	// bUseLobbiesIfAvailable이 다르면 즉시 UnknownError로 실패시킨다.
	// 그런데 로비 데이터로 실제 전송되는 세션 플래그 비트필드
	// (GetLobbyKeyValuePairsFromSessionSettings)에는 bUseLobbiesIfAvailable이 빠져 있다.
	// → 검색으로 복원한 결과는 항상 bUsesPresence=true / bUseLobbiesIfAvailable=false가 되어
	//   "방은 목록에 보이는데 입장은 100% 실패"한다.
	// 호스트는 CreateSessionInternal에서 두 값을 같게 만들어 광고하므로,
	// 복원된 값도 같게 맞춰주면 원래 의도대로 동작한다.
	DesiredSession.Session.SessionSettings.bUseLobbiesIfAvailable =
		DesiredSession.Session.SessionSettings.bUsesPresence;

	// PrintNS(FString::Printf(TEXT("Joining session [%d]..."), SearchResultIndex));
	if (!SessionInterface->JoinSession(0, NAME_GameSession, DesiredSession))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		// PrintNS(TEXT("JoinSession request REJECTED"), FColor::Red);
		bJoinInProgress = false;
		OnJoinSessionCompleteEvent.Broadcast(false);
	}
}

void USISessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		// PrintNS(FString::Printf(TEXT("Join Complete: FAIL (%d)"), static_cast<int32>(Result)), FColor::Red);

		// 실패 시 로컬에 등록된 세션 정리 → 재시도 가능하게 ("can't join twice" 방지)
		if (SessionInterface->GetNamedSession(NAME_GameSession))
		{
			// PrintNS(TEXT("Cleaning up local session after failed join"));
			DestroySession();
		}
		bJoinInProgress = false;
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	if (!PC)
	{
		// PrintNS(TEXT("Travel FAIL: PlayerController null"), FColor::Red);
		bJoinInProgress = false;
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		// PrintNS(TEXT("Travel FAIL: could not resolve connect string"), FColor::Red);
		bJoinInProgress = false;
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}
	
	// 비밀번호가 있으면 URL 옵션으로 동봉 → 서버의 PreLogin이 읽는다
	if (!PendingJoinPassword.IsEmpty())
	{
		ConnectString += FString::Printf(TEXT("?Password=%s"), *PendingJoinPassword);
	}

	// PrintNS(FString::Printf(TEXT("Traveling to: %s"), *ConnectString), FColor::Green);
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

	// PrintNS(TEXT("Destroying session..."));
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		// PrintNS(TEXT("DestroySession request REJECTED"), FColor::Red);
		bCreateSessionOnDestroy = false;
		OnDestroySessionCompleteEvent.Broadcast(false);
	}
}

void USISessionSubsystem::LeaveSession()
{
	if (!EnsureSessionInterface())
	{
		OnSessionLeftEvent.Broadcast(ESISessionLeaveReason::UserRequested);
		return;
	}

	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		// PrintNS(TEXT("LeaveSession: no session to leave"));
		OnSessionLeftEvent.Broadcast(ESISessionLeaveReason::UserRequested);   // 정리할 게 없어도 "나갔다"는 사실은 방송
		return;
	}

	bLeaveInProgress = true;
	bJoinInProgress = false;
	PendingLeaveReason = ESISessionLeaveReason::UserRequested;
	DestroySession();
}

void USISessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	// PrintNS(FString::Printf(TEXT("Destroy Complete: %s"), bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAIL")));

	OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);

	// Destroy → Create 체인의 연결 고리
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSessionInternal();
	}
	
	// "나가는 중"이었다면 의미적 이벤트로 완료 통지 — 후속 흐름은 구독자의 몫
	if (bLeaveInProgress)
	{
		bLeaveInProgress = false;
		OnSessionLeftEvent.Broadcast(PendingLeaveReason);
	}
}

#pragma endregion

#pragma region NetworkState

void USISessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	// PrintNS(FString::Printf(TEXT("Network Failure: %s"), *ErrorString), FColor::Red);

	// 0) 접속 거절(PendingConnectionFailure)은 "내가 참가하기를 누른 경우"에만 내 일이다.
	//    OnNetworkFailure는 엔진 전역 델리게이트라 PIE에서 클라를 여러 개 띄우면
	//    GameInstance마다 있는 이 서브시스템 전부에게 남의 실패까지 배달된다.
	//    콜백이 주는 World 인자는 아직 월드가 확정되기 전(PendingNetGame)이라 믿을 수 없어서,
	//    우리만 아는 사실인 "내가 시도했다"(bJoinInProgress)를 기준으로 삼는다.
	if (FailureType == ENetworkFailure::PendingConnectionFailure && !bJoinInProgress)
	{
		return;
	}

	// 1) 내가 스스로 나가는 중이라면 그때 발생하는 연결 해제는 "실패"가 아니다.
	//    기록해두면 메인메뉴에서 엉뚱하게 "연결이 끊어졌습니다" 안내가 뜬다.
	if (bLeaveInProgress && PendingLeaveReason == ESISessionLeaveReason::UserRequested)
	{
		// PrintNS(TEXT("Ignoring failure — voluntary leave in progress"));
		return;
	}

	// 2) 실패를 의미 단위로 분류
	ESIConnectionFailureType Classified = ESIConnectionFailureType::Unknown;
	if (FailureType == ENetworkFailure::PendingConnectionFailure
		&& ErrorString.Contains(SISessionErrors::WrongPassword))   // 상수 안 쓰면 TEXT("Incorrect password")
	{
		Classified = ESIConnectionFailureType::WrongPassword;
	}
	else if (FailureType == ENetworkFailure::PendingConnectionFailure
		&& ErrorString.Contains(SISessionErrors::GameInProgress))
	{
		Classified = ESIConnectionFailureType::GameInProgress;
	}
	else if (FailureType == ENetworkFailure::ConnectionLost
		  || FailureType == ENetworkFailure::ConnectionTimeout)
	{
		Classified = ESIConnectionFailureType::ConnectionLost;
	}

	// 3) 우편함에 보관 (후속 실패가 덮어써도 무해)
	LastFailureType = Classified;
	LastFailureRawMessage = ErrorString;
	bHasPendingFailure = true;

	const bool bHasLiveSession =
		SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;

	// 4) 접속 거절은 엔진이 알아서 기본 맵으로 되돌린다("Connection failed; returning to Entry").
	//    우리까지 OpenLevel을 부르면 맵을 두 번 로드하게 되므로, 세션 정리만 하고 빠진다.
	//    안내는 메인메뉴 도착 후 ConsumeLastFailure를 읽는 쪽이 담당한다.
	if (FailureType == ENetworkFailure::PendingConnectionFailure)
	{
		bJoinInProgress = false;

		if (bHasLiveSession)
		{
			DestroySession();   // bLeaveInProgress를 세우지 않는다 = 우리 쪽 복귀 처리 없음
		}

		return;
	}

	// 5) 그 밖의 실패는 이미 방 안에 있다가 끊긴 것 — 세션 정리 후 메인메뉴로 되돌린다.
	//    세션이 없다 = 이미 첫 실패가 정리/통지를 끝냈다는 뜻이므로, 후속 실패는 기록만 하고 조용히 무시
	if (bHasLiveSession)
	{
		bLeaveInProgress = true;
		PendingLeaveReason = ESISessionLeaveReason::ConnectionLost;
		DestroySession();
	}
}

bool USISessionSubsystem::ConsumeLastFailure(ESIConnectionFailureType& OutType, FString& OutRawMessage)
{
	if (!bHasPendingFailure)
	{
		return false;
	}
	OutType = LastFailureType;
	OutRawMessage = LastFailureRawMessage;
	bHasPendingFailure = false;   // 1회성 — 다음 메인메뉴 진입 때 옛 실패가 또 뜨지 않게
	return true;
}

#pragma endregion








