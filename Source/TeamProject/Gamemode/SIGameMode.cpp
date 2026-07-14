#include "SIGameMode.h"

#include "Engine/DataTable.h"
#include "GameFramework/PlayerState.h"
#include "GameState/SIGameState.h"
#include "PlayerController/SIPlayerController.h"
#include "PlayerState/SIPlayerState.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr int32 MaxPlayers = 8;
	constexpr int32 MaxChatLength = 256;

	bool IsKeywordProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return false;
		}

		const FString PropertyName = Property->GetName();
		const FString DisplayName = Property->GetDisplayNameText().ToString();
		return PropertyName.Equals(TEXT("Keyword"), ESearchCase::IgnoreCase)
			|| PropertyName.StartsWith(TEXT("Keyword_"), ESearchCase::IgnoreCase)
			|| DisplayName.Equals(TEXT("Keyword"), ESearchCase::IgnoreCase);
	}

	FString ReadKeywordValue(const FProperty* Property, const uint8* RowData)
	{
		if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
		{
			return StringProperty->GetPropertyValue_InContainer(RowData);
		}

		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			return TextProperty->GetPropertyValue_InContainer(RowData).ToString();
		}

		if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			return NameProperty->GetPropertyValue_InContainer(RowData).ToString();
		}

		return FString();
	}
}

ASIGameMode::ASIGameMode()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> KeywordTableFinder(
		TEXT("/Game/Shape_It/Game/Keyword.Keyword"));
	if (KeywordTableFinder.Succeeded())
	{
		KeywordDataTable = KeywordTableFinder.Object;
	}
}

void ASIGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsValid(NewPlayer) || PlayerOrderList.Num() >= MaxPlayers)
	{
		return;
	}

	PlayerOrderList.Add(NewPlayer);

	if (ASIPlayerState* PlayerState = NewPlayer->GetPlayerState<ASIPlayerState>())
	{
		PlayerState->bIsHost = PlayerOrderList.Num() == 1;

		if (ASIGameState* SIState = GetGameState<ASIGameState>())
		{
			SIState->Multicast_BroadcastPlayerJoined(PlayerState);
		}
	}
}

void ASIGameMode::Logout(AController* Exiting)
{
	APlayerController* ExitingPlayer = Cast<APlayerController>(Exiting);
	APlayerState* ExitingPlayerState = IsValid(Exiting) ? Exiting->PlayerState : nullptr;
	const int32 RemovedIndex = PlayerOrderList.IndexOfByKey(ExitingPlayer);
	const ASIGameState* CurrentSIState = GetGameState<ASIGameState>();
	const bool bRemovedCurrentWorkspaceOwner = RemovedIndex == CurrentWorkspaceIndex
		&& CurrentSIState
		&& CurrentSIState->CurrentGamePhase == ESIGamePhase::GuessPhase;

	if (RemovedIndex != INDEX_NONE)
	{
		PlayerOrderList.RemoveAt(RemovedIndex);
		PlayerAssignedWords.Remove(ExitingPlayer);
		CorrectPlayersThisTurn.Remove(ExitingPlayer);

		if (RemovedIndex < CurrentWorkspaceIndex)
		{
			--CurrentWorkspaceIndex;
		}
	}

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		if (IsValid(ExitingPlayerState))
		{
			SIState->Multicast_BroadcastPlayerLeft(ExitingPlayerState);
		}

		SIState->TotalRounds = PlayerOrderList.Num();
	}

	if (!PlayerOrderList.IsEmpty())
	{
		if (ASIPlayerState* NewHostState = PlayerOrderList[0]->GetPlayerState<ASIPlayerState>())
		{
			NewHostState->bIsHost = true;
		}
	}

	Super::Logout(Exiting);

	if (bRemovedCurrentWorkspaceOwner)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		GetWorldTimerManager().SetTimerForNextTick(this, &ASIGameMode::StartNextGuessTurn);
	}
}

bool ASIGameMode::AssignWordsToPlayers()
{
	PlayerAssignedWords.Empty();

	if (!KeywordDataTable || !KeywordDataTable->GetRowStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] Keyword DataTable이 설정되지 않았습니다."));
		return false;
	}

	const FProperty* KeywordProperty = nullptr;
	for (TFieldIterator<FProperty> PropertyIt(KeywordDataTable->GetRowStruct()); PropertyIt; ++PropertyIt)
	{
		if (IsKeywordProperty(*PropertyIt))
		{
			KeywordProperty = *PropertyIt;
			break;
		}
	}

	if (!KeywordProperty)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] DataTable Row에서 Keyword 필드를 찾지 못했습니다."));
		return false;
	}

	TArray<FString> CandidateWords;
	TSet<FString> UniqueWords;
	for (const TPair<FName, uint8*>& RowPair : KeywordDataTable->GetRowMap())
	{
		FString Word = ReadKeywordValue(KeywordProperty, RowPair.Value).TrimStartAndEnd();
		const FString ComparisonKey = Word.ToLower();
		if (!Word.IsEmpty() && !UniqueWords.Contains(ComparisonKey))
		{
			UniqueWords.Add(ComparisonKey);
			CandidateWords.Add(MoveTemp(Word));
		}
	}

	if (CandidateWords.Num() < PlayerOrderList.Num())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GameMode] 제시어가 부족합니다. 필요: %d, 유효 제시어: %d"),
			PlayerOrderList.Num(), CandidateWords.Num());
		return false;
	}

	for (int32 Index = CandidateWords.Num() - 1; Index > 0; --Index)
	{
		CandidateWords.Swap(Index, FMath::RandRange(0, Index));
	}

	for (int32 Index = 0; Index < PlayerOrderList.Num(); ++Index)
	{
		APlayerController* Player = PlayerOrderList[Index];
		if (!IsValid(Player))
		{
			PlayerAssignedWords.Empty();
			return false;
		}

		PlayerAssignedWords.Add(Player, CandidateWords[Index]);
	}

	return true;
}

void ASIGameMode::StartGameMatch()
{
	ASIGameState* SIState = GetGameState<ASIGameState>();
	if (!SIState || PlayerOrderList.IsEmpty())
	{
		return;
	}

	if (SIState->CurrentGamePhase == ESIGamePhase::BuildPhase
		|| SIState->CurrentGamePhase == ESIGamePhase::GuessPhase)
	{
		return;
	}

	if (!AssignWordsToPlayers())
	{
		return;
	}

	SpawnPlayersToIndividualWorkspaces();

	for (APlayerController* Player : PlayerOrderList)
	{
		const FString* AssignedWord = PlayerAssignedWords.Find(Player);
		if (AssignedWord)
		{
			if (ASIPlayerController* SIPlayer = Cast<ASIPlayerController>(Player))
			{
				SIPlayer->Client_ReceiveSecretWord(*AssignedWord);
			}
		}
	}

	CurrentWorkspaceIndex = -1;
	CorrectPlayersThisTurn.Empty();
	SIState->CurrentRound = 0;
	SIState->TotalRounds = PlayerOrderList.Num();
	SIState->SetCurrentWorkspaceOwner(nullptr);
	SIState->SetRemainingTime(FMath::CeilToInt(BuildTimeLimit));
	SIState->SetGamePhase(ESIGamePhase::BuildPhase);

	GetWorldTimerManager().SetTimer(
		GameTimerHandle, this, &ASIGameMode::EndBuildPhase, BuildTimeLimit, false);
	GetWorldTimerManager().SetTimer(
		UITimerTickHandle, this, &ASIGameMode::OnUITimerTick, 1.0f, true);
}

void ASIGameMode::OnUITimerTick()
{
	ASIGameState* SIState = GetGameState<ASIGameState>();
	if (!SIState)
	{
		return;
	}

	const float TimerRemaining = GetWorldTimerManager().GetTimerRemaining(GameTimerHandle);
	SIState->SetRemainingTime(FMath::Max(FMath::CeilToInt(TimerRemaining), 0));
}

void ASIGameMode::EndBuildPhase()
{
	CurrentWorkspaceIndex = 0;
	StartNextGuessTurn();
}

void ASIGameMode::StartNextGuessTurn()
{
	if (!PlayerOrderList.IsValidIndex(CurrentWorkspaceIndex))
	{
		EndMatch();
		return;
	}

	APlayerController* WorkspaceOwner = PlayerOrderList[CurrentWorkspaceIndex];
	if (!IsValid(WorkspaceOwner) || !PlayerAssignedWords.Contains(WorkspaceOwner))
	{
		++CurrentWorkspaceIndex;
		StartNextGuessTurn();
		return;
	}

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		SIState->CurrentRound = CurrentWorkspaceIndex + 1;
		SIState->TotalRounds = PlayerOrderList.Num();
		SIState->SetCurrentWorkspaceOwner(WorkspaceOwner->PlayerState);
		SIState->SetRemainingTime(FMath::CeilToInt(GuessTimeLimit));
		SIState->SetGamePhase(ESIGamePhase::GuessPhase);
	}

	SpawnPlayersToTargetWorkspace(WorkspaceOwner);
	CorrectPlayersThisTurn.Empty();

	GetWorldTimerManager().SetTimer(
		GameTimerHandle, this, &ASIGameMode::EndGuessTurn, GuessTimeLimit, false);
}

void ASIGameMode::EndGuessTurn()
{
	++CurrentWorkspaceIndex;
	StartNextGuessTurn();
}

void ASIGameMode::OnChatReceived(APlayerController* Sender, const FString& Message)
{
	ASIGameState* SIState = GetGameState<ASIGameState>();
	if (!SIState || !IsValid(Sender) || !PlayerOrderList.Contains(Sender))
	{
		return;
	}

	FString NormalizedMessage = Message.TrimStartAndEnd();
	if (NormalizedMessage.IsEmpty())
	{
		return;
	}
	NormalizedMessage.LeftInline(MaxChatLength);

	if (SIState->CurrentGamePhase != ESIGamePhase::GuessPhase)
	{
		BroadcastChat(Sender, NormalizedMessage);
		return;
	}

	if (!PlayerOrderList.IsValidIndex(CurrentWorkspaceIndex))
	{
		return;
	}

	APlayerController* WorkspaceOwner = PlayerOrderList[CurrentWorkspaceIndex];
	const FString* CorrectAnswer = PlayerAssignedWords.Find(WorkspaceOwner);
	if (!IsValid(WorkspaceOwner) || !CorrectAnswer)
	{
		return;
	}

	const bool bMatchesAnswer = NormalizedMessage.Equals(*CorrectAnswer, ESearchCase::IgnoreCase);
	if (!bMatchesAnswer)
	{
		BroadcastChat(Sender, NormalizedMessage);
		return;
	}

	// 정답 텍스트는 제출 자격과 무관하게 절대 채팅으로 내보내지 않습니다.
	if (Sender == WorkspaceOwner || CorrectPlayersThisTurn.Contains(Sender))
	{
		return;
	}

	ASIPlayerState* SenderState = Sender->GetPlayerState<ASIPlayerState>();
	if (!SenderState)
	{
		return;
	}

	const int32 ScoreToEarn = FMath::Max(5 - CorrectPlayersThisTurn.Num(), 1);
	SenderState->AddScore(ScoreToEarn);
	CorrectPlayersThisTurn.Add(Sender);

	FAnswerResultPayload Payload;
	Payload.Submitter = SenderState;
	Payload.SubmittedAnswer = TEXT("");
	Payload.ScoreEarned = ScoreToEarn;
	Payload.bIsCorrect = true;
	SIState->Multicast_BroadcastAnswerResult(Payload);
}

void ASIGameMode::BroadcastChat(APlayerController* Sender, const FString& Message)
{
	if (!IsValid(Sender))
	{
		return;
	}

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		FChatMessagePayload Payload;
		Payload.Sender = Sender->PlayerState;
		Payload.Message = Message;
		SIState->Multicast_BroadcastChatMessage(Payload);
	}
}

void ASIGameMode::EndMatch()
{
	GetWorldTimerManager().ClearTimer(GameTimerHandle);
	GetWorldTimerManager().ClearTimer(UITimerTickHandle);

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		SIState->SetRemainingTime(0);
		SIState->SetCurrentWorkspaceOwner(nullptr);
		SIState->SetGamePhase(ESIGamePhase::ResultPhase);
		SIState->Multicast_BroadcastMatchEnded();
	}

	FTimerHandle ReturnTimerHandle;
	GetWorldTimerManager().SetTimer(ReturnTimerHandle, [this]()
	{
		ProcessServerTravel(TEXT("/Game/Shape_It/Level/Test_Lobby?listen"));
	}, 5.0f, false);
}
