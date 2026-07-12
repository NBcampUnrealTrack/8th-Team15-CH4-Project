#include "SIGameMode.h"
#include "GameState/SIGameState.h"
#include "PlayerController/SIPlayerController.h"
#include "PlayerState/SIPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

ASIGameMode::ASIGameMode()
{
	// GameStateClass = ASIGameState::StaticClass();
	// PlayerControllerClass = ASIPlayerController::StaticClass();

	CurrentWorkspaceIndex = -1;
}

void ASIGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 최대 정원 8명 제한 유지
	if (NewPlayer && PlayerOrderList.Num() < 8)
	{
		PlayerOrderList.Add(NewPlayer);
	}
}

void ASIGameMode::StartGameMatch()
{
	// 1. 모든 플레이어를 찢어서 각자의 방으로 보냅니다. (블루프린트 이벤트 호출)
	SpawnPlayersToIndividualWorkspaces();

	ASIGameState* SIGameState = GetGameState<ASIGameState>();
	if (SIGameState)
	{
		SIGameState->CurrentGamePhase = ESIGamePhase::BuildPhase;
	}

	// 2. 제작 시간 타이머 시작
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ASIGameMode::EndBuildPhase, BuildTimeLimit, false);
	UE_LOG(LogTemp, Warning, TEXT("[Server] %f초 동안의 개인 제작 시간이 시작되었습니다."), BuildTimeLimit);
}

void ASIGameMode::EndBuildPhase()
{
	// 제작이 끝나면 1번 플레이어(인덱스 0) 방부터 투어를 시작합니다.
	CurrentWorkspaceIndex = 0;
	StartNextGuessTurn();
}

void ASIGameMode::StartNextGuessTurn()
{
	// 모든 방을 다 돌았다면 게임 종료
	if (!PlayerOrderList.IsValidIndex(CurrentWorkspaceIndex))
	{
		EndMatch();
		return;
	}

	ASIGameState* SIGameState = GetGameState<ASIGameState>();
	if (SIGameState)
	{
		SIGameState->CurrentGamePhase = ESIGamePhase::GuessPhase;
		SIGameState->CurrentPresenter = PlayerOrderList[CurrentWorkspaceIndex]->PlayerState;
	}

	// 모든 플레이어를 이번 턴 주인의 방으로 강제 스폰시킵니다.
	APlayerController* TargetOwner = PlayerOrderList[CurrentWorkspaceIndex];
	SpawnPlayersToTargetWorkspace(TargetOwner);

	// 임시 정답 세팅 (추후 데이터테이블 등과 연동)
	CurrentCorrectAnswer = TEXT("사과");
	CorrectCountThisTurn = 0;

	// 정답 맞추기 시간 타이머 시작
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ASIGameMode::EndGuessTurn, GuessTimeLimit, false);
	UE_LOG(LogTemp, Warning, TEXT("[Server] %d번 플레이어 방 투어 시작! (%f초)"), CurrentWorkspaceIndex + 1, GuessTimeLimit);
}

void ASIGameMode::EndGuessTurn()
{
	// 시간이 다 되면 무조건 다음 방으로 넘어갑니다.
	CurrentWorkspaceIndex++;
	StartNextGuessTurn();
}

void ASIGameMode::OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer)
{
	ASIGameState* SIGameState = GetGameState<ASIGameState>();

	// 맞추기 단계가 아니면 무시
	if (SIGameState && SIGameState->CurrentGamePhase != ESIGamePhase::GuessPhase) return;

	// 출제자(방 주인) 본인은 정답 제출 금지
	if (Submitter == PlayerOrderList[CurrentWorkspaceIndex]) return;

	// 정답 판별 (대소문자 무시)
	if (SubmittedAnswer.Equals(CurrentCorrectAnswer, ESearchCase::IgnoreCase))
	{
		ASIPlayerState* SubmitterState = Submitter->GetPlayerState<ASIPlayerState>();
		if (SubmitterState)
		{
			// 선착순 점수 차등 지급 (AddScore 사용해야 OnScoreUpdated 델리게이트가 발동됨)
			int32 ScoreToEarn = FMath::Max(5 - CorrectCountThisTurn, 1);
			SubmitterState->AddScore(ScoreToEarn);

			// 방 주인에게도 1점 보너스 지급
			if (ASIPlayerState* PresenterState = Cast<ASIPlayerState>(SIGameState->CurrentPresenter))
			{
				PresenterState->AddScore(1);
			}

			CorrectCountThisTurn++;
		}
	}
}

void ASIGameMode::EndMatch()
{
	ASIGameState* SIGameState = GetGameState<ASIGameState>();
	if (SIGameState)
	{
		SIGameState->CurrentGamePhase = ESIGamePhase::ResultPhase;
	}

	// 5초 대기 후 로비 맵으로 강제 귀환
	FTimerHandle ReturnTimerHandle;
	GetWorldTimerManager().SetTimer(ReturnTimerHandle, [this]()
		{
			ProcessServerTravel(TEXT("/Game/Maps/LobbyMap?listen"));
		}, 5.f, false);
}