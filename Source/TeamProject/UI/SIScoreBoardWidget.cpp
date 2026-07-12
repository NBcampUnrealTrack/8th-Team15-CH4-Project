// SIScoreBoardWidget.cpp

#include "UI/SIScoreBoardWidget.h"
#include "GameState/SIGameState.h"
#include "PlayerState/SIPlayerState.h"

void USIScoreBoardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ASIGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASIGameState>() : nullptr;
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ScoreBoardWidget] GameState 미확보 - 델리게이트 바인딩 실패"));
		return;
	}

	CachedGameState = GS;

	GS->OnPlayerJoined.AddDynamic(this, &USIScoreBoardWidget::HandlePlayerJoined);
	GS->OnPlayerLeft.AddDynamic(this, &USIScoreBoardWidget::HandlePlayerLeft);
	GS->OnMatchEnded.AddDynamic(this, &USIScoreBoardWidget::HandleMatchEnded);

	// 이미 접속해 있는 플레이어들의 OnScoreUpdated에 바인딩
	for (APlayerState* PS : GS->PlayerArray)
	{
		BindPlayerScoreDelegate(PS);
	}

	RefreshSortedPlayers();
}

void USIScoreBoardWidget::NativeDestruct()
{
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPlayerJoined.RemoveDynamic(this, &USIScoreBoardWidget::HandlePlayerJoined);
		CachedGameState->OnPlayerLeft.RemoveDynamic(this, &USIScoreBoardWidget::HandlePlayerLeft);
		CachedGameState->OnMatchEnded.RemoveDynamic(this, &USIScoreBoardWidget::HandleMatchEnded);

		for (APlayerState* PS : CachedGameState->PlayerArray)
		{
			UnbindPlayerScoreDelegate(PS);
		}
	}

	Super::NativeDestruct();
}

void USIScoreBoardWidget::HandleAnyScoreChanged(int32 NewScore)
{
	RefreshSortedPlayers();
}

void USIScoreBoardWidget::HandlePlayerJoined(APlayerState* JoinedPlayer)
{
	BindPlayerScoreDelegate(JoinedPlayer);
	RefreshSortedPlayers();
}

void USIScoreBoardWidget::HandlePlayerLeft(APlayerState* LeftPlayer)
{
	UnbindPlayerScoreDelegate(LeftPlayer);
	RefreshSortedPlayers();
}

void USIScoreBoardWidget::HandleMatchEnded()
{
	bMatchEnded = true;
	RefreshSortedPlayers();
	OnFinalRankingReady();
}

void USIScoreBoardWidget::RefreshSortedPlayers()
{
	SortedPlayers.Reset();
	if (!CachedGameState.IsValid())
	{
		return;
	}

	for (APlayerState* PS : CachedGameState->PlayerArray)
	{
		if (PS)
		{
			SortedPlayers.Add(PS);
		}
	}

	SortedPlayers.Sort([](const TObjectPtr<APlayerState>& A, const TObjectPtr<APlayerState>& B)
	{
		const ASIPlayerState* SA = Cast<ASIPlayerState>(A.Get());
		const ASIPlayerState* SB = Cast<ASIPlayerState>(B.Get());
		const int32 ScoreA = SA ? SA->CurrentScore : 0;
		const int32 ScoreB = SB ? SB->CurrentScore : 0;
		return ScoreA > ScoreB;
	});

	OnScoreBoardRefreshed();
}

void USIScoreBoardWidget::BindPlayerScoreDelegate(APlayerState* PS)
{
	if (ASIPlayerState* SIPS = Cast<ASIPlayerState>(PS))
	{
		SIPS->OnScoreUpdated.AddDynamic(this, &USIScoreBoardWidget::HandleAnyScoreChanged);
	}
}

void USIScoreBoardWidget::UnbindPlayerScoreDelegate(APlayerState* PS)
{
	if (ASIPlayerState* SIPS = Cast<ASIPlayerState>(PS))
	{
		SIPS->OnScoreUpdated.RemoveDynamic(this, &USIScoreBoardWidget::HandleAnyScoreChanged);
	}
}
