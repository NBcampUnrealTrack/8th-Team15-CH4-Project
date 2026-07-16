// SIParticipantsListWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SIParticipantsListWidget.generated.h"

class ASIGameState;
class APlayerState;

class UVerticalBox;

UCLASS()
class TEAMPROJECT_API USIParticipantsListWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	// 현재 접속 중인 참가자 목록 (접속 순서 유지)
	UPROPERTY(BlueprintReadOnly, Category = "Participants")
	TArray<TObjectPtr<APlayerState>> Participants;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_ParticipantsList;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePlayerJoined(APlayerState* JoinedPlayer);

	UFUNCTION()
	void HandlePlayerLeft(APlayerState* LeftPlayer);
	
	void DrawParticipants();
	void RefreshParticipantsFromGameState();

	TWeakObjectPtr<ASIGameState> CachedGameState;
};
