// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SISessionSubsystem.generated.h"

namespace SISessionErrors
{
	static const FString WrongPassword = TEXT("Incorrect password");

	/** 이미 시작된 매치에 뒤늦게 붙으려 할 때. 서버가 PreLogin에서 채우고,
		클라이언트는 HandleNetworkFailure에서 이 문자열을 보고 사유를 분류한다. */
	static const FString GameInProgress = TEXT("Game already in progress");
}

/** 방 설정 허용 범위 — 로비의 서버 검증과 GameMode의 적용이 같은 값을 봐야 한다 */
namespace SIRoomSettingLimits
{
	static constexpr float MinBuildTime = 30.0f;
	static constexpr float MaxBuildTime = 600.0f;
	static constexpr int32 MinPlacedShapeCount = 10;
	static constexpr int32 MaxPlacedShapeCount = 40;
	static constexpr int32 DefaultPlacedShapeCount = 25;
}

/** 방 생성 시 UI에서 받아오는 설정 묶음 */
USTRUCT(BlueprintType)
struct FSICreateSessionParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Session")
	FString RoomTitle = TEXT("New Room");

	UPROPERTY(BlueprintReadWrite, Category = "Session")
	int32 MaxPlayers = 8;

	/** 광고하지 않음 — 호스트 측 검증용으로만 보관. 비어 있으면 공개방 */
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	FString Password;
	
	/** 그리기 제한 시간(초). 0 이하 = GameMode 기본값 사용 */
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	float BuildTime = 0.0f;

	/** 플레이어 한 명이 설치할 수 있는 최대 도형 개수 */
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	int32 MaxPlacedShapeCount = SIRoomSettingLimits::DefaultPlacedShapeCount;
};

/** BP로 내보낼 검색 결과 요약 (FOnlineSessionSearchResult는 USTRUCT가 아니라 직접 노출 불가) */
USTRUCT(BlueprintType)
struct FSISessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString RoomTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	/** 자물쇠 표시 + 입장 시 비번 입력창 노출 기준 */
	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bHasPassword = false;

	/** true = 이미 게임이 진행 중인 방 (bAllowJoinInProgress라 입장 자체는 가능) */
	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bIsInProgress = false;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 SearchResultIndex = INDEX_NONE;
};

/** 방을 떠나게 된 이유 — UI가 안내 문구를 구분하는 데 사용 */
UENUM(BlueprintType)
enum class ESISessionLeaveReason : uint8
{
	UserRequested   UMETA(DisplayName = "유저가 나가기 선택"),
	ConnectionLost  UMETA(DisplayName = "연결 끊김/호스트 이탈")
};

/** 접속/연결 실패의 의미 분류 — UI가 안내 문구를 결정하는 기준 */
UENUM(BlueprintType)
enum class ESIConnectionFailureType : uint8
{
	WrongPassword    UMETA(DisplayName = "비밀번호 불일치"),
	ConnectionLost   UMETA(DisplayName = "연결 끊김 (호스트 이탈 등)"),
	GameInProgress   UMETA(DisplayName = "이미 시작된 게임 (중도 참여 불가)"),
	Unknown          UMETA(DisplayName = "기타 오류")
};

/** ── UI(BP)로 결과를 방송하는 번역 계층 ── */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSIOnFindSessionsComplete, const TArray<FSISessionInfo>&, Sessions, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIOnSessionLeft, ESISessionLeaveReason, Reason);


UCLASS()
class TEAMPROJECT_API USISessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
#pragma region UGameInstanceSubsystem Override
	
public:
	/** ── 수명 ── */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	virtual void Deinitialize() override;
	
#pragma endregion
	
#pragma region Online
	
public:
	/** ── 공개 API (UI/레벨이 호출하는 것) ── */
	UFUNCTION(BlueprintCallable, Category = "SI|Session")
    void CreateSession(const FSICreateSessionParams& Params);

	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	void FindSessions(int32 MaxSearchResults = 20);

	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	void JoinSessionByIndex(int32 SearchResultIndex, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	void DestroySession();

	/** 현재 세션에서 나간다. 정리 완료 후 OnDestroySessionCompleteEvent 방송 →
		구독자(UI/레벨)가 메인메뉴 복귀를 실행 */
	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	void LeaveSession();
	
	/** 방이 게임 중인지를 세션에 광고한다 — 목록에 "게임중/대기중"으로 뜬다.
		호스트에서만 의미가 있다(참가자는 세션 설정을 들고 있지 않아 조용히 무시됨).
		각 GameMode가 BeginPlay에서 자기 상태를 선언하는 식으로 쓴다. */
	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	void SetSessionInProgress(bool bInProgress);

	/** 로비에서 호스트가 방 설정을 바꿨을 때 — 보관값(PendingParams)을 갈아끼우고
		목록에 광고되는 방 제목/자물쇠 표시도 함께 갱신한다. 호스트에서만 의미가 있다. */
	void UpdateHostSessionParams(const FSICreateSessionParams& NewParams);

	/** PreLogin 검증용 — LobbyGameMode가 읽어감 */
	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	const FSICreateSessionParams& GetHostSessionParams() const { return PendingParams; }

	/** 마지막 연결 실패 정보를 꺼내간다 (1회성 — 읽으면 비워짐).
		실패가 없었으면 false 반환. MainMenu가 도착 직후 호출해 안내 팝업에 사용 */
	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	bool ConsumeLastFailure(ESIConnectionFailureType& OutType, FString& OutRawMessage);
	
	/** ── UI가 구독하는 방송 채널 ── */
	UPROPERTY(BlueprintAssignable, Category = "SI|Session")
	FSIOnCreateSessionComplete OnCreateSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "SI|Session")
	FSIOnDestroySessionComplete OnDestroySessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "SI|Session")
	FSIOnFindSessionsComplete OnFindSessionsCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "SI|Session")
	FSIOnJoinSessionComplete OnJoinSessionCompleteEvent;

	/** "방에서 나오게 됐다"는 의미적 이벤트. 세션 정리가 끝난 뒤 방송된다.
		구독자(레벨/UI)가 메인메뉴 복귀 등 후속 흐름을 결정한다. */
	UPROPERTY(BlueprintAssignable, Category = "SI|Session")
	FSIOnSessionLeft OnSessionLeftEvent;
	
protected:
	/** ── OSS 완료 콜백 (내부용) ── */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	
	// GEngine->OnNetworkFailure() 콜백
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType, const FString& ErrorString);

	/** 맵 로딩 완료 = 입장 시도가 끝났다는 뜻. bJoinInProgress를 내린다.
		이 시점의 World는 확정된 상태라 소유 GameInstance 비교를 신뢰할 수 있다. */
	void HandlePostLoadMap(UWorld* LoadedWorld);
	
public:
	/** 현재 붙어 있는 온라인 서브시스템이 LAN 전용(Null)인가.
		Null이면 방 광고/검색이 UDP 브로드캐스트라 같은 공유기 안에서만 보이고,
		Steam이면 밸브 로비 서버를 거치므로 인터넷 너머까지 보인다.
		CreateSession/FindSessions가 이 값으로 bIsLANMatch·bIsLanQuery를 정한다.
		→ ini의 DefaultPlatformService만 바꿔도 코드 수정 없이 전환된다. */
	UFUNCTION(BlueprintPure, Category = "SI|Session")
	bool IsLanOnlySubsystem() const;

private:
	/** 세션 인터페이스 지연 획득 (Initialize 시점엔 World가 아직 없을 수 있음) */
	bool EnsureSessionInterface();

	/** 실제 생성 로직 (Destroy 체인의 도착점) */
	void CreateSessionInternal();

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	/** Destroy 완료 후 이어서 할 일 플래그 */
	bool bCreateSessionOnDestroy = false;
	
	/** 기존 PendingNumPublicConnections를 대체 — 방 설정 전체를 보관 */
	FSICreateSessionParams PendingParams;
	
	/** Join 시에 입력된 Password 입력값 보관 */
	FString PendingJoinPassword;

	/** 이 인스턴스가 직접 입장을 시도했는가.
		OnNetworkFailure는 엔진 전역이라 PIE에서 남의 실패까지 전부 흘러들어온다.
		"내가 참가하기를 눌렀다"는 건 우리만 아는 사실이므로 이걸 기준으로 걸러낸다. */
	bool bJoinInProgress = false;

	/** bReturnToMainMenuOnDestroy 를 대체 — "나가는 중" 상태와 이유 */
	bool bLeaveInProgress = false;
	ESISessionLeaveReason PendingLeaveReason = ESISessionLeaveReason::UserRequested;
	
	bool bHasPendingFailure = false;
	ESIConnectionFailureType LastFailureType = ESIConnectionFailureType::Unknown;
	FString LastFailureRawMessage;
	
#pragma endregion
};
