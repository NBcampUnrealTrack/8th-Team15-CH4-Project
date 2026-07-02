/** SIGameState.h */
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SIGameState.generated.h"

/**
*게임의 현재 진행 상태를 나타내는 열거형
*상태 머신 기반으로 게임의 흐름을 제어하는 데 사용
*/
UENUM(BlueprintType)
enum class EMatchState : uint8
{
	WaitingToStart, // 대기실: 플레이어 입장 및 게임 시작 대기
	InProgress, // 진행 중: 플레이어들이 순서대로 출제하고 정답을 맞추는 상태
	GameOver, // 종료: 최종 승리자 산출 및 결과 연출
	ReturningToLobby // 로비 복귀: 게임이 끝나고 로비로 돌아가는 중
};

/**
*3D 캐치마인드 방식 게임의 글로벌 상태를 관리하는 GameState 클래스
*서버와 모든 클라이언트에게 공유되며, 현재 출제자, 남은 시간, 게임 상태 등을 동기화.
*/
UCLASS()
class TEAMPROJECT_API ASIGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASIGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 현재 게임의 진행 상태 (대기, 진행, 종료 등) */
	UPROPERTY(ReplicatedUsing = OnRep_MatchState, BlueprintReadOnly, Category = "Match")
	EMatchState CurrentMatchState;

	/** 현재 턴에서 문제를 출제하고 있는 플레이어의 상태 정보*/
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	APlayerState* CurrentPresenter;

	/** 현재 라운드의 남은 시간 (초 단위) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	float RemainingTime;

protected:
	/** MatchState가 변경될 때 클라이언트에서 호출되는 이벤트 함수 */
	UFUNCTION()
	void OnRep_MatchState();
};