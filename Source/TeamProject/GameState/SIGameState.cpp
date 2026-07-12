#include "SIGameState.h"
#include "Net/UnrealNetwork.h"

ASIGameState::ASIGameState()
{
	CurrentGamePhase = ESIGamePhase::None;
	RemainingTime = 0;
	CurrentPresenter = nullptr;
	CurrentRound = 1;
	TotalRounds = 1;
}

void ASIGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASIGameState, CurrentGamePhase);
	DOREPLIFETIME(ASIGameState, RemainingTime);
	DOREPLIFETIME(ASIGameState, CurrentPresenter);
	DOREPLIFETIME(ASIGameState, CurrentRound);
	DOREPLIFETIME(ASIGameState, TotalRounds);
}

// ==========================================
// [OnRep 함수 - 상태 변경 브로드캐스트]
// ==========================================

void ASIGameState::OnRep_GamePhase()
{
	if (OnPhaseChanged.IsBound())
	{
		OnPhaseChanged.Broadcast(CurrentGamePhase);
	}
	UE_LOG(LogTemp, Warning, TEXT("[GameState] 게임 단계가 변경되었습니다."));
}

void ASIGameState::OnRep_RemainingTime()
{
	if (OnTimeUpdated.IsBound())
	{
		OnTimeUpdated.Broadcast(RemainingTime);
	}
}

void ASIGameState::OnRep_CurrentPresenter()
{
	if (OnPresenterChanged.IsBound())
	{
		OnPresenterChanged.Broadcast(CurrentPresenter);
	}
}

// ==========================================
// [Multicast RPC - 순간 이벤트 브로드캐스트]
// 서버가 호출하면 모든 클라이언트에서 실행되어 델리게이트를 발동시킴
// ==========================================

void ASIGameState::Multicast_BroadcastAnswerResult_Implementation(const FAnswerResultPayload& Payload)
{
	if (OnAnswerResult.IsBound())
	{
		OnAnswerResult.Broadcast(Payload);
	}
}

void ASIGameState::Multicast_BroadcastMatchEnded_Implementation()
{
	if (OnMatchEnded.IsBound())
	{
		OnMatchEnded.Broadcast();
	}
}

void ASIGameState::Multicast_BroadcastChatMessage_Implementation(const FChatMessagePayload& Payload)
{
	if (OnChatMessage.IsBound())
	{
		OnChatMessage.Broadcast(Payload);
	}
}

void ASIGameState::Multicast_BroadcastPlayerJoined_Implementation(APlayerState* JoinedPlayer)
{
	if (OnPlayerJoined.IsBound())
	{
		OnPlayerJoined.Broadcast(JoinedPlayer);
	}
}

void ASIGameState::Multicast_BroadcastPlayerLeft_Implementation(APlayerState* LeftPlayer)
{
	if (OnPlayerLeft.IsBound())
	{
		OnPlayerLeft.Broadcast(LeftPlayer);
	}
}
