#include "SIGameState.h"
#include "Net/UnrealNetwork.h"

ASIGameState::ASIGameState()
{
	// 초기값 세팅
	CurrentGamePhase = ESIGamePhase::None;
	RemainingTime = 0;
	CurrentPresenter = nullptr;
	CurrentRound = 1;
	TotalRounds = 1;
}

void ASIGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 서버의 변수값을 클라이언트들에게 복제(동기화)하도록 등록
	DOREPLIFETIME(ASIGameState, CurrentGamePhase);
	DOREPLIFETIME(ASIGameState, RemainingTime);
	DOREPLIFETIME(ASIGameState, CurrentPresenter);
	DOREPLIFETIME(ASIGameState, CurrentRound);
	DOREPLIFETIME(ASIGameState, TotalRounds);
}

void ASIGameState::OnRep_GamePhase()
{
	// 서버로부터 새로운 페이즈 정보를 받았을 때, UI나 연출 블루프린트에 이벤트를 날림.
	OnPhaseChanged.Broadcast(CurrentGamePhase);

	UE_LOG(LogTemp, Warning, TEXT("[GameState] 게임 단계가 변경되었습니다."));
}

void ASIGameState::OnRep_RemainingTime()
{
	// 서버로부터 타이머가 1초씩 줄어들 때마다 UI 블루프린트에 이벤트를 날려줍니다.
	// 덕분에 클라이언트 UI 팀원은 매 틱(Tick)마다 검사할 필요 없이 이 이벤트만 받아서 화면의 숫자를 바꾸면 됩니다.
	OnTimeUpdated.Broadcast(RemainingTime);
}

// ==========================================
// ★ 새로 추가된 부분: 방 주인이 바뀌었을 때 클라이언트에서 실행되는 함수
// ==========================================
void ASIGameState::OnRep_CurrentPresenter()
{
	// UI 블루프린트에 만들어둔 이벤트 노드에 신호를 보냅니다.
	// "방 주인이 CurrentPresenter로 바뀌었어! UI 텍스트 바꿔!"
	if (OnPresenterChanged.IsBound())
	{
		OnPresenterChanged.Broadcast(CurrentPresenter);
	}
}