// SIGameState.cpp
#include "SIGameState.h"
#include "Net/UnrealNetwork.h"

ASIGameState::ASIGameState()
{
	CurrentMatchState = EMatchState::WaitingToStart;
	CurrentPresenter = nullptr;
	RemainingTime = 0.0f;
}

void ASIGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 복제할 변수들 등록 (서버 -> 클라이언트)
	DOREPLIFETIME(ASIGameState, CurrentMatchState);
	DOREPLIFETIME(ASIGameState, CurrentPresenter);
	DOREPLIFETIME(ASIGameState, RemainingTime);
}

void ASIGameState::OnRep_MatchState()
{
	// 클라이언트에서 상태가 변했을 때 UI 업데이트나 연출을 실행 가능
	// 예: EMatchState::GameOver가 되면 클라이언트 화면에 "게임 종료" UI 띄우기
}