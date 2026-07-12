#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Enums/SITypes.h"
#include "SIGameState.generated.h"

// ==========================================
// [델리게이트 선언 - 상태 변경 이벤트 (Replicated 변수의 OnRep에서 Broadcast)]
// ==========================================

// 게임 페이즈 변경
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, ESIGamePhase, NewPhase);

// 남은 시간 갱신
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdatedSignature, int32, NewTime);

// 현재 방 주인(발표자) 변경
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPresenterChangedSignature, class APlayerState*, NewPresenter);

// ==========================================
// [델리게이트 선언 - 순간 이벤트 (Multicast RPC에서 Broadcast)]
// ==========================================

// 정답/오답 결과 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnswerResultSignature, const FAnswerResultPayload&, Payload);

// 매치 종료 (최종 순위 표시 신호)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchEndedSignature);

// 채팅 메시지 도착
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageSignature, const FChatMessagePayload&, Payload);

// 플레이어 참가/이탈 (참가자 리스트 갱신용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerJoinedSignature, class APlayerState*, JoinedPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeftSignature, class APlayerState*, LeftPlayer);


/**
 * 게임의 전반적인 상태(타이머, 현재 페이즈, 발표자, 순간 이벤트)를 모든 클라이언트와 동기화하는 클래스입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASIGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ==========================================
	// [상태 변수 - Replicated]
	// ==========================================

	// 현재 게임 페이즈
	UPROPERTY(ReplicatedUsing = OnRep_GamePhase, Transient, BlueprintReadOnly, Category = "GamePhase")
	ESIGamePhase CurrentGamePhase;

	UFUNCTION()
	void OnRep_GamePhase();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPhaseChangedSignature OnPhaseChanged;

	// 남은 시간(초)
	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, Transient, BlueprintReadOnly, Category = "GameTimer")
	int32 RemainingTime;

	UFUNCTION()
	void OnRep_RemainingTime();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTimeUpdatedSignature OnTimeUpdated;

	// 현재 발표자(구경 중인 방의 주인)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPresenter, Transient, BlueprintReadOnly, Category = "GameData")
	class APlayerState* CurrentPresenter;

	UFUNCTION()
	void OnRep_CurrentPresenter();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPresenterChangedSignature OnPresenterChanged;

	// 현재 라운드 번호 / 총 라운드 수
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 CurrentRound;

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 TotalRounds;

	// ==========================================
	// [순간 이벤트 - Multicast RPC로 트리거]
	// 서버가 아래 Multicast_* 함수 호출 → 모든 클라이언트에서 대응 델리게이트 Broadcast
	// ==========================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAnswerResultSignature OnAnswerResult;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastAnswerResult(const FAnswerResultPayload& Payload);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMatchEndedSignature OnMatchEnded;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastMatchEnded();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnChatMessageSignature OnChatMessage;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastChatMessage(const FChatMessagePayload& Payload);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerJoinedSignature OnPlayerJoined;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastPlayerJoined(class APlayerState* JoinedPlayer);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerLeftSignature OnPlayerLeft;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastPlayerLeft(class APlayerState* LeftPlayer);
};
