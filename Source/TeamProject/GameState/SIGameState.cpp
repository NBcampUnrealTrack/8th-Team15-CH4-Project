
#include "SIGameState.h"
#include "GameInstance/SIGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASIGameState::ASIGameState()
{
	CurrentGamePhase = ESIGamePhase::None;
	RemainingTime = 0;
	CurrentWorkspaceOwner = nullptr;
	CurrentRound = 0;
	TotalRounds = 0;
}

void ASIGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASIGameState, CurrentGamePhase);
	DOREPLIFETIME(ASIGameState, RemainingTime);
	DOREPLIFETIME(ASIGameState, CurrentWorkspaceOwner);
	DOREPLIFETIME(ASIGameState, CurrentRound);
	DOREPLIFETIME(ASIGameState, TotalRounds);
	DOREPLIFETIME(ASIGameState, LobbyRoomTitle);
	DOREPLIFETIME(ASIGameState, LobbyMaxPlayers);
	DOREPLIFETIME(ASIGameState, LobbyBuildTime);
	DOREPLIFETIME(ASIGameState, LobbyGuessTime);
}

void ASIGameState::OnRep_LobbyRoomInfo()
{
	OnLobbyRoomInfoChanged.Broadcast();
}

void ASIGameState::SetLobbyRoomInfo(const FString& InRoomTitle, const int32 InMaxPlayers,
	const float InBuildTime, const float InGuessTime)
{
	if (!HasAuthority())
	{
		return;
	}

	LobbyRoomTitle = InRoomTitle;
	LobbyMaxPlayers = InMaxPlayers;
	LobbyBuildTime = InBuildTime;
	LobbyGuessTime = InGuessTime;

	// 리슨 서버(호스트)에는 OnRep이 호출되지 않으므로 직접 알립니다.
	OnRep_LobbyRoomInfo();
	ForceNetUpdate();
}

void ASIGameState::SetGamePhase(const ESIGamePhase NewPhase)
{
	if (!HasAuthority() || CurrentGamePhase == NewPhase)
	{
		return;
	}

	CurrentGamePhase = NewPhase;
	OnRep_GamePhase();
	ForceNetUpdate();
}

void ASIGameState::SetRemainingTime(const int32 NewRemainingTime)
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 ClampedTime = FMath::Max(NewRemainingTime, 0);
	if (RemainingTime == ClampedTime)
	{
		return;
	}

	RemainingTime = ClampedTime;
	OnRep_RemainingTime();
	ForceNetUpdate();
}

void ASIGameState::SetCurrentWorkspaceOwner(APlayerState* NewWorkspaceOwner)
{
	if (!HasAuthority() || CurrentWorkspaceOwner == NewWorkspaceOwner)
	{
		return;
	}

	CurrentWorkspaceOwner = NewWorkspaceOwner;
	OnRep_CurrentWorkspaceOwner();
	ForceNetUpdate();
}

void ASIGameState::Multicast_BroadcastScoreboardUpdated_Implementation()
{
	OnScoreboardUpdated.Broadcast();
}


void ASIGameState::OnRep_GamePhase()
{
	OnPhaseChanged.Broadcast(CurrentGamePhase);
}

void ASIGameState::OnRep_RemainingTime()
{
	OnTimeUpdated.Broadcast(RemainingTime);
}

void ASIGameState::OnRep_CurrentWorkspaceOwner()
{
	OnWorkspaceOwnerChanged.Broadcast(CurrentWorkspaceOwner);
}

void ASIGameState::Multicast_BroadcastAnswerResult_Implementation(const FAnswerResultPayload& Payload)
{
	OnAnswerResult.Broadcast(Payload);
}

void ASIGameState::Multicast_BroadcastMatchEnded_Implementation()
{
	OnMatchEnded.Broadcast();
}

void ASIGameState::Multicast_ShowLoadingScreen_Implementation()
{
	// 데디케이티드 서버엔 띄울 화면이 없다
	if (USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>())
	{
		SIInstance->ShowLoadingScreen();
	}
}

void ASIGameState::Multicast_BroadcastChatMessage_Implementation(const FChatMessagePayload& Payload)
{
	OnChatMessage.Broadcast(Payload);
}

void ASIGameState::Multicast_BroadcastPlayerJoined_Implementation(APlayerState* JoinedPlayer)
{
	OnPlayerJoined.Broadcast(JoinedPlayer);
}

void ASIGameState::Multicast_BroadcastPlayerLeft_Implementation(APlayerState* LeftPlayer)
{
	OnPlayerLeft.Broadcast(LeftPlayer);
}

#pragma region Sound 

void ASIGameState::Mulitcast_GameStartSound_Implementation()
{
	if (GameStartSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), GameStartSound);
	}
}

void ASIGameState::Multicast_PlayerCorrectAnswerSound_Implementation(AActor* CorrectPlayer)
{
	if (!CorrectPlayer || !CorrectAnswerSound) return;
	
	UGameplayStatics::SpawnSoundAttached
	(
	CorrectAnswerSound,
	CorrectPlayer->GetRootComponent(),
	NAME_None,
	FVector::ZeroVector,
	EAttachLocation::KeepRelativeOffset,
	false,
	1.0f, 1.0f, 0.0f,
	CorrectAttenuation
	);
}
#pragma endregion