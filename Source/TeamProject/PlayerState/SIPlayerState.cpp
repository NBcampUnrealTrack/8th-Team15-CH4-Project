#include "SIPlayerState.h"
#include "Gamemode/SIGameMode.h" // GameMode의 StartGameMatch를 호출하기 위해 포함
#include "Net/UnrealNetwork.h"

ASIPlayerState::ASIPlayerState()
{
	// 초기값 세팅
	CurrentScore = 0;
	bIsHost = false; // 기본값은 호스트가 아님으로 설정
}

void ASIPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 서버의 변수 값을 모든 클라이언트들에게 복제(동기화)하도록 등록
	DOREPLIFETIME(ASIPlayerState, CurrentScore);
	DOREPLIFETIME(ASIPlayerState, bIsHost); // 호스트 여부도 다른 클라이언트들이 알 수 있도록 동기화
}

// ----------------------------------------------------
// 점수 증가 및 동기화 로직
// ----------------------------------------------------

void ASIPlayerState::AddScore(int32 Amount)
{
	// 점수 조작은 오직 서버에서만 가능하도록 권한(Authority) 체크
	if (HasAuthority())
	{
		CurrentScore += Amount;

		// 호스트(서버 본인)의 화면 UI도 즉시 업데이트하기 위해 수동으로 한 번 호출
		OnRep_CurrentScore();
	}
}

void ASIPlayerState::OnRep_CurrentScore()
{
	// 점수가 바뀔 때마다 UI 블루프린트에 이벤트를 날림
	if (OnScoreUpdated.IsBound())
	{
		OnScoreUpdated.Broadcast(CurrentScore);
	}
}

// ----------------------------------------------------
// 호스트 게임 시작 요청 로직 (Server RPC)
// ----------------------------------------------------

void ASIPlayerState::Server_RequestStartGame_Implementation()
{
	// 서버 측에서 한 번 더 이 요청을 보낸 플레이어가 진짜 호스트가 맞는지 엄격하게 검증 
	if (!bIsHost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 호스트 권한이 없는 플레이어가 게임 시작을 시도했습니다."));
		return;
	}

	// 서버에만 유일하게 존재하는 매치 심판(GameMode)을 가져옵니다.
	if (ASIGameMode* GameMode = GetWorld()->GetAuthGameMode<ASIGameMode>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 호스트의 정상적인 요청으로 게임을 시작합니다."));

		// 이전에 개편해 두었던 게임모드의 동시 제작 및 순회 매치 루프를 가동합니다.
		GameMode->StartGameMatch();
	}
}