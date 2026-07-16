// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SISessionSubsystem.generated.h"

/** 방 생성 시 UI에서 받아오는 설정 묶음 */
USTRUCT(BlueprintType)
struct FSICreateSessionParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Session")
	FString RoomTitle = TEXT("New Room");

	UPROPERTY(BlueprintReadWrite, Category = "Session")
	int32 MaxPlayers = 5;

	UPROPERTY(BlueprintReadWrite, Category = "Session")
	bool bIsPrivate = false;

	/** 광고하지 않음 — 호스트 측 검증용으로만 보관 */
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	FString Password;
	
	/** 그리기 제한 시간(초). 0 이하 = GameMode 기본값 사용 */
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	float BuildTime = 0.0f;

	/** 정답 제한 시간(초). 0 이하 = GameMode 기본값 사용 */
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	float GuessTime = 0.0f;
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

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bIsPrivate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 SearchResultIndex = INDEX_NONE;
};

/** ── UI(BP)로 결과를 방송하는 번역 계층 ── */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSIOnFindSessionsComplete, const TArray<FSISessionInfo>&, Sessions, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIOnJoinSessionComplete, bool, bWasSuccessful);

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

	UFUNCTION(BlueprintCallable, Category = "NS|Session")
	void FindSessions(int32 MaxSearchResults = 20);

	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	void JoinSessionByIndex(int32 SearchResultIndex, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "NS|Session")
	void DestroySession();
	
	/** PreLogin 검증용 — LobbyGameMode가 읽어감 */
	UFUNCTION(BlueprintCallable, Category = "SI|Session")
	const FSICreateSessionParams& GetHostSessionParams() const { return PendingParams; }

	
	/** ── UI가 구독하는 방송 채널 ── */
	UPROPERTY(BlueprintAssignable, Category = "NS|Session")
	FSIOnCreateSessionComplete OnCreateSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "NS|Session")
	FSIOnDestroySessionComplete OnDestroySessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "NS|Session")
	FSIOnFindSessionsComplete OnFindSessionsCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "NS|Session")
	FSIOnJoinSessionComplete OnJoinSessionCompleteEvent;

	/** 접속 실패/추방 통지 — UI가 "비밀번호가 틀렸습니다" 등을 띄우는 데 사용 */
	UPROPERTY(BlueprintAssignable, Category = "SI|Session")
	FSIOnJoinSessionComplete OnNetworkFailureEvent;   // FString 파라미터 델리게이트를 따로 선언해도 좋음
	
protected:
	/** ── OSS 완료 콜백 (내부용) ── */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	
	// GEngine->OnNetworkFailure() 콜백
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType, const FString& ErrorString);
	
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
	
#pragma endregion
};
