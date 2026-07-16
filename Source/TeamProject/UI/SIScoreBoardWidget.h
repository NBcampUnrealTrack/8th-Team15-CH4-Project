// SIScoreBoardWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SIScoreBoardWidget.generated.h"

class ASIGameState;
class ASIPlayerState;
class APlayerState;

class UVerticalBox;

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


private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_Rankings;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void DrawScoreBoard();
	
	UFUNCTION()
	void HandleScoreboardUpdated();

	UFUNCTION()
	void HandlePlayerJoined(APlayerState* JoinedPlayer);

	UFUNCTION()
	void HandlePlayerLeft(APlayerState* LeftPlayer);

	UFUNCTION()
	void HandleMatchEnded();

	// 현재 PlayerArray를 스코어 내림차순으로 정렬해 SortedPlayers 갱신
	void RefreshSortedPlayers();



	TWeakObjectPtr<ASIGameState> CachedGameState;
};
