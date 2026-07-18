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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyRoomInfoChangedSignature);


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

	// ── 로비 방 설정 ──
	// 원본은 호스트 GameInstance의 USISessionSubsystem::GetHostSessionParams().
	// 클라이언트는 그 값을 알 방법이 없으므로 GameState가 복제해 UI에 전달합니다.
	// (Password는 검증용이라 복제하지 않습니다 — 호스트만 자기 서브시스템에서 직접 읽습니다.)
	UPROPERTY(ReplicatedUsing = OnRep_LobbyRoomInfo, Transient, BlueprintReadOnly, Category = "Lobby")
	FString LobbyRoomTitle;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyRoomInfo, Transient, BlueprintReadOnly, Category = "Lobby")
	int32 LobbyMaxPlayers = 0;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyRoomInfo, Transient, BlueprintReadOnly, Category = "Lobby")
	float LobbyBuildTime = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyRoomInfo, Transient, BlueprintReadOnly, Category = "Lobby")
	float LobbyGuessTime = 0.0f;

	UFUNCTION()
	void OnRep_LobbyRoomInfo();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLobbyRoomInfoChangedSignature OnLobbyRoomInfoChanged;

	void SetLobbyRoomInfo(const FString& InRoomTitle, int32 InMaxPlayers, float InBuildTime, float InGuessTime);

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
