#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Enums/SITypes.h"
#include "SIGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, ESIGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdatedSignature, int32, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorkspaceOwnerChangedSignature, class APlayerState*, NewWorkspaceOwner);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnswerResultSignature, const FAnswerResultPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScoreboardUpdatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageSignature, const FChatMessagePayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerJoinedSignature, class APlayerState*, JoinedPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeftSignature, class APlayerState*, LeftPlayer);


class USoundBase;

UCLASS()
class TEAMPROJECT_API ASIGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASIGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_GamePhase, Transient, BlueprintReadOnly, Category = "GamePhase")
	ESIGamePhase CurrentGamePhase;

	UFUNCTION()
	void OnRep_GamePhase();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, Transient, BlueprintReadOnly, Category = "GameTimer")
	int32 RemainingTime;

	UFUNCTION()
	void OnRep_RemainingTime();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTimeUpdatedSignature OnTimeUpdated;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWorkspaceOwner, Transient, BlueprintReadOnly, Category = "GameData")
	class APlayerState* CurrentWorkspaceOwner;

	UFUNCTION()
	void OnRep_CurrentWorkspaceOwner();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWorkspaceOwnerChangedSignature OnWorkspaceOwnerChanged;

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 CurrentRound;

	UPROPERTY(Replicated, Transient, BlueprintReadOnly, Category = "GameData")
	int32 TotalRounds;

	// 서버에서 값을 변경하면서 Listen Server의 로컬 UI에도 즉시 알립니다.
	void SetGamePhase(ESIGamePhase NewPhase);
	void SetRemainingTime(int32 NewRemainingTime);
	void SetCurrentWorkspaceOwner(APlayerState* NewWorkspaceOwner);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAnswerResultSignature OnAnswerResult;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastAnswerResult(const FAnswerResultPayload& Payload);
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnScoreboardUpdatedSignature OnScoreboardUpdated;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastScoreboardUpdated();

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
	void Multicast_BroadcastPlayerJoined(APlayerState* JoinedPlayer);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerLeftSignature OnPlayerLeft;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastPlayerLeft(APlayerState* LeftPlayer);
	
#pragma region Sound
	
	UFUNCTION(NetMulticast, Reliable)
	void Mulitcast_GameStartSound();
	
	//파라미터로 AActor 받는 이유는 실제 월드상에 존재하는 액터의 위치를 받기 위해서 참조합니다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayerCorrectAnswerSound(AActor* CorrectPlayer);
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> GameStartSound;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> CorrectAnswerSound;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundAttenuation> CorrectAttenuation;

#pragma endregion
};
