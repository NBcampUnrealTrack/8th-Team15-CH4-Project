// SIScoreBoardWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SIScoreBoardWidget.generated.h"

class ASIGameState;
class ASIPlayerState;
class APlayerState;

/**
 * 점수판. 모든 PlayerState의 OnScoreUpdated에 바인딩되어 점수 변화 시 리프레시하며,
 * 매치 종료 시 최종 순위를 정렬해 표시합니다.
 */
UCLASS()
class TEAMPROJECT_API USIScoreBoardWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	// 현재 점수 순으로 정렬된 플레이어 리스트 (BP에서 위젯 리스트 갱신에 사용)
	UPROPERTY(BlueprintReadOnly, Category = "ScoreBoard")
	TArray<TObjectPtr<APlayerState>> SortedPlayers;

	// 매치 종료 상태 (true면 최종 순위 표시 모드)
	UPROPERTY(BlueprintReadOnly, Category = "ScoreBoard")
	bool bMatchEnded = false;

	// 스코어 리스트가 갱신될 때 BP에서 UI를 다시 그리도록 오버라이드
	UFUNCTION(BlueprintImplementableEvent, Category = "ScoreBoard")
	void OnScoreBoardRefreshed();

	// 매치 종료 시 최종 순위 발표용
	UFUNCTION(BlueprintImplementableEvent, Category = "ScoreBoard")
	void OnFinalRankingReady();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 어떤 PlayerState의 스코어가 바뀌든 전체 순회 리프레시
	UFUNCTION()
	void HandleAnyScoreChanged(int32 NewScore);

	UFUNCTION()
	void HandlePlayerJoined(APlayerState* JoinedPlayer);

	UFUNCTION()
	void HandlePlayerLeft(APlayerState* LeftPlayer);

	UFUNCTION()
	void HandleMatchEnded();

	// 현재 PlayerArray를 스코어 내림차순으로 정렬해 SortedPlayers 갱신
	void RefreshSortedPlayers();

	// 개별 PlayerState의 OnScoreUpdated에 바인딩
	void BindPlayerScoreDelegate(APlayerState* PS);
	void UnbindPlayerScoreDelegate(APlayerState* PS);

	TWeakObjectPtr<ASIGameState> CachedGameState;
};
