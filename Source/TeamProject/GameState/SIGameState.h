#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h" // AGameStateBase -> AGameState로 변경
#include "SIGameState.generated.h"


UENUM(BlueprintType)
enum class ESIGamePhase : uint8
{
	None UMETA(DisplayName = "대기 중"),
	BuildPhase UMETA(DisplayName = "제작 단계"),
	GuessPhase UMETA(DisplayName = "정답 맞추기 단계"),
	ResultPhase UMETA(DisplayName = "결과 발표")
};

// UI 블루프린트에 값이 변했음을 즉시 알려주기 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, ESIGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdatedSignature, int32, NewTime);
// ★ 추가: 우리가 구경하는 방 주인이 바뀔 때 UI에 알려주는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPresenterChangedSignature, class APlayerState*, NewPresenter);

/**
 * 게임의 전반적인 상태(타이머, 현재 페이즈, 점수 등)를 모든 클라이언트와 동기화하는 클래스입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASIGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ==========================================
	// [게임 페이즈(단계) 관리]
	// ==========================================

	// 현재 게임이 어느 단계인지 나타냅니다. 값이 바뀌면 OnRep_GamePhase가 실행됩니다.
	UPROPERTY(ReplicatedUsing = OnRep_GamePhase, Transient, BlueprintReadOnly, Category = "GamePhase")
	ESIGamePhase CurrentGamePhase;

	UFUNCTION()
	void OnRep_GamePhase();

	// 블루프린트에서 Event 노드로 뺄 수 있는 델리게이트 (UI 업데이트용)
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPhaseChangedSignature OnPhaseChanged;

	// ==========================================
	// [타이머 관리]
	// ==========================================

	// 남은 시간 (초). 값이 바뀌면 OnRep_RemainingTime이 실행됩니다.
	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, Transient, BlueprintReadOnly, Category = "GameTimer")
	int32 RemainingTime;

	UFUNCTION()
	void OnRep_RemainingTime();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTimeUpdatedSignature OnTimeUpdated;

	// ==========================================
	// [게임 데이터 동기화 (투어 관리)]
	// ==========================================

	// ★ 수정: 방 주인이 바뀔 때마다 OnRep 함수를 통해 클라이언트 UI를 갱신하도록 변경
	// (현재 우리가 구경하고 있는 작업 공간의 주인)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPresenter, Transient, BlueprintReadOnly, Category = "GameData")
	class APlayerState* CurrentPresenter;

	UFUNCTION()
	void OnRep_CurrentPresenter();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPresenterChangedSignature OnPresenterChanged;

	// 현재 구경 중인 방의 순서 (예: 1번째 방, 2번째 방...)
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 CurrentRound;

	// 총 돌아야 할 방의 개수 (총 플레이어 수와 동일)
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 TotalRounds;
};
