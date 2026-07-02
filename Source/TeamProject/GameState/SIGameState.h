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
	// [게임 데이터 동기화]
	// ==========================================

	// 현재 출제자 (도형을 만들었거나, 현재 작업 공간의 주인)
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	class APlayerState* CurrentPresenter;

	// 현재 라운드 진행도 (몇 번째 플레이어의 턴인지)
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 CurrentRound;

	// 총 라운드 수 (총 플레이어 수와 동일하게 설정됨)
	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 TotalRounds;
};