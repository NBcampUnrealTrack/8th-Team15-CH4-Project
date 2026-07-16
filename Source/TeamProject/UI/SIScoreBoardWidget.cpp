// SIScoreBoardWidget.cpp

#include "UI/SIScoreBoardWidget.h"
#include "GameState/SIGameState.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
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
	GS->OnScoreboardUpdated.AddDynamic(this, &USIScoreBoardWidget::HandleScoreboardUpdated);
	GS->OnMatchEnded.AddDynamic(this, &USIScoreBoardWidget::HandleMatchEnded);

	RefreshSortedPlayers();
}

void USIScoreBoardWidget::NativeDestruct()
{
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPlayerJoined.RemoveDynamic(this, &USIScoreBoardWidget::HandlePlayerJoined);
		CachedGameState->OnPlayerLeft.RemoveDynamic(this, &USIScoreBoardWidget::HandlePlayerLeft);
		CachedGameState->OnScoreboardUpdated.RemoveDynamic(this, &USIScoreBoardWidget::HandleScoreboardUpdated);
		CachedGameState->OnMatchEnded.RemoveDynamic(this, &USIScoreBoardWidget::HandleMatchEnded);
	}

	Super::NativeDestruct();
}

void USIScoreBoardWidget::DrawScoreBoard()
{
	if (!VerticalBox_Rankings)
	{
		return;
	}
	
	const int32 SlotCount = VerticalBox_Rankings->GetChildrenCount();
	
	int32 DisplayRank = 1;
	
	for (int32 i = 0; i < SlotCount; ++i)
	{
		UTextBlock* RawText = Cast<UTextBlock>(VerticalBox_Rankings->GetChildAt(i));
		if (!RawText)
		{
			continue;
		}

		if (i < SortedPlayers.Num())
		{
			const FString PlayerName = SortedPlayers[i]->GetPlayerName();
			
			ASIPlayerState* PS = Cast<ASIPlayerState>(SortedPlayers[i]);
			
			if (!IsValid(PS))
			{
				continue;
			}
			
			const int32 PlayerScore = PS->CurrentScore;
			
			if (i > 0)
			{
				ASIPlayerState* PrevPS = Cast<ASIPlayerState>(SortedPlayers[i - 1]);
				const int32 PrevScore = PrevPS ? PrevPS->CurrentScore : 0;

				if (PlayerScore != PrevScore)   // 앞 사람과 점수가 다르면
				{
					DisplayRank = i + 1;        // 등수를 현재 위치로 갱신
				}
				// 점수가 같으면 DisplayRank 그대로 유지 → 공동순위
			}
			
			const FString Line = FString::Printf(TEXT("%d. %s : %d"), DisplayRank, *PlayerName, PlayerScore);
			RawText->SetText(FText::FromString(Line));
			RawText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			RawText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USIScoreBoardWidget::HandleScoreboardUpdated()
{
	RefreshSortedPlayers();
}

void USIScoreBoardWidget::HandlePlayerJoined(APlayerState* JoinedPlayer)
{
	RefreshSortedPlayers();
}

void USIScoreBoardWidget::HandlePlayerLeft(APlayerState* LeftPlayer)
{
	RefreshSortedPlayers();
}

void USIScoreBoardWidget::HandleMatchEnded()
{
	bMatchEnded = true;
	RefreshSortedPlayers();
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

	DrawScoreBoard();
}

