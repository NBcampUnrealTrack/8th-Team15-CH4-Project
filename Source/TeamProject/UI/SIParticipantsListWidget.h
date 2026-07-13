// SIParticipantsListWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SIParticipantsListWidget.generated.h"

class ASIGameState;
class APlayerState;


UCLASS()
class TEAMPROJECT_API USIParticipantsListWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	// 현재 접속 중인 참가자 목록 (접속 순서 유지)
	UPROPERTY(BlueprintReadOnly, Category = "Participants")
	TArray<TObjectPtr<APlayerState>> Participants;

	// 리스트가 갱신될 때 BP에서 UI를 다시 그리도록 오버라이드
	UFUNCTION(BlueprintImplementableEvent, Category = "Participants")
	void OnParticipantsRefreshed();

	// 점수 갱신 알림 (BP에서 필요 시 정렬 등)
	UFUNCTION(BlueprintImplementableEvent, Category = "Participants")
	void OnParticipantScoreChanged();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePlayerJoined(APlayerState* JoinedPlayer);

	UFUNCTION()
	void HandlePlayerLeft(APlayerState* LeftPlayer);

	UFUNCTION()
	void HandleAnyScoreChanged(int32 NewScore);

	void RefreshParticipantsFromGameState();
	void BindPlayerScoreDelegate(APlayerState* PS);
	void UnbindPlayerScoreDelegate(APlayerState* PS);

	TWeakObjectPtr<ASIGameState> CachedGameState;
};
