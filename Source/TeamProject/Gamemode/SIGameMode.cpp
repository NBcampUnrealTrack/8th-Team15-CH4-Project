#include "SIGameMode.h"

#include "Character/SICharacter.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "GameInstance/SIGameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameState/SIGameState.h"
#include "PlayerController/SIPlayerController.h"
#include "PlayerState/SIPlayerState.h"
#include "TimerManager.h"
#include "GameInstance/SISessionSubsystem.h"
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

void ASIGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		if (SIState->CurrentGamePhase == ESIGamePhase::None)
		{
			SIState->SetGamePhase(ESIGamePhase::LobbyPhase);
		}
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

	TryStartPendingMatch();
}

void ASIGameMode::TryStartPendingMatch()
{
	USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>();
	if (!SIInstance || !SIInstance->IsPendingMatchReady(PlayerOrderList.Num()))
	{
		return;
	}

	// 같은 프레임에 여러 PostLogin이 들어와도 시작 예약은 한 번만 만듭니다.
	SIInstance->ConsumePendingMatch();
	GetWorldTimerManager().SetTimerForNextTick(this, &ASIGameMode::StartGameMatch);
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
		PlayerWorkspaceAreas.Remove(ExitingPlayer);

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

void ASIGameMode::RegisterPlayerWorkspace(APlayerController* Player, AActor* WorkspaceArea)
{
	if (!HasAuthority() || !IsValid(Player) || !IsValid(WorkspaceArea))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameMode][Workspace] 등록 실패: Player=%s, Area=%s"),
			*GetNameSafe(Player), *GetNameSafe(WorkspaceArea));
		return;
	}

	if (!PlayerOrderList.Contains(Player))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameMode][Workspace] 게임 참가자가 아닌 Controller입니다: %s"),
			*GetNameSafe(Player));
		return;
	}

	PlayerWorkspaceAreas.Add(Player, WorkspaceArea);
	UE_LOG(LogTemp, Log,
		TEXT("[GameMode][Workspace] 등록 완료: %s -> %s"),
		*GetNameSafe(Player), *GetNameSafe(WorkspaceArea));
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
	
	// ★ 방 설정 반영 (이후의 BuildTimeLimit/GuessTimeLimit 사용처가 전부 이 값을 씀)
	ApplyHostMatchSettings();   

	if (!AssignWordsToPlayers())
	{
		return;
	}

	PlayerWorkspaceAreas.Empty();
	SpawnPlayersToIndividualWorkspaces();

	for (APlayerController* Player : PlayerOrderList)
	{
		if (!PlayerWorkspaceAreas.Contains(Player))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameMode][Workspace] %s의 작업공간이 등록되지 않았습니다. "
					"BP의 Individual 배치 직후 RegisterPlayerWorkspace를 호출해야 합니다."),
				*GetNameSafe(Player));
		}
	}

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

void ASIGameMode::ApplyHostMatchSettings()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const USISessionSubsystem* Subsystem =
		GameInstance ? GameInstance->GetSubsystem<USISessionSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;   // 서브시스템 없으면 기본값 유지
	}

	const FSICreateSessionParams& HostParams = Subsystem->GetHostSessionParams();

	// 센티널(0 이하) = 미지정 → 기본값 유지. 지정됐어도 서버가 상식 범위로 clamp
	if (HostParams.BuildTime > 0.0f)
	{
		BuildTimeLimit = FMath::Clamp(HostParams.BuildTime, 30.0f, 600.0f);
	}
	if (HostParams.GuessTime > 0.0f)
	{
		GuessTimeLimit = FMath::Clamp(HostParams.GuessTime, 10.0f, 120.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Match settings: Build=%.0fs, Guess=%.0fs"),
		BuildTimeLimit, GuessTimeLimit);
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
	for (APlayerController* Player : PlayerOrderList)
	{
		if (IsValid(Player))
		{
			if (ASICharacter* SICharacter = Cast<ASICharacter>(Player->GetPawn()))
			{
				SICharacter->RestoreActiveShapeEditForPhaseChange();
			}
		}
	}

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

void ASIGameMode::SpawnPlayersToTargetWorkspace(APlayerController* TargetOwner)
{
	if (!HasAuthority() || !IsValid(TargetOwner))
	{
		return;
	}

	const TObjectPtr<AActor>* TargetAreaPtr = PlayerWorkspaceAreas.Find(TargetOwner);
	AActor* TargetArea = TargetAreaPtr ? TargetAreaPtr->Get() : nullptr;
	if (!IsValid(TargetArea))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GameMode][GuessSpawn] %s에게 등록된 WorkspaceArea가 없습니다."),
			*GetNameSafe(TargetOwner));
		return;
	}

	TArray<AActor*> ViewingAreaActors;
	UGameplayStatics::GetAllActorsWithTag(this, GuessViewingAreaTag, ViewingAreaActors);
	if (ViewingAreaActors.IsEmpty() || !IsValid(ViewingAreaActors[0]))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GameMode][GuessSpawn] Actor Tag '%s'가 붙은 중앙 관람 기준점을 찾지 못했습니다."),
			*GuessViewingAreaTag.ToString());
		return;
	}

	if (ViewingAreaActors.Num() > 1)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameMode][GuessSpawn] Actor Tag '%s' 기준점이 %d개입니다. 첫 번째 액터를 사용합니다."),
			*GuessViewingAreaTag.ToString(), ViewingAreaActors.Num());
	}

	const AActor* ViewingArea = ViewingAreaActors[0];
	const FVector CenterLocation = ViewingArea->GetActorLocation();
	const FVector TargetLocation = TargetArea->GetActorLocation();
	FVector ViewDirection = TargetLocation - CenterLocation;
	ViewDirection.Z = 0.0f;
	ViewDirection = ViewDirection.GetSafeNormal();
	if (ViewDirection.IsNearlyZero())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GameMode][GuessSpawn] 중앙 기준점과 목표 WorkspaceArea의 XY 위치가 같습니다."));
		return;
	}

	const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, ViewDirection).GetSafeNormal();
	const FVector FormationAnchor = CenterLocation + ViewDirection * GuessFormationOffsetTowardTarget;
	const FVector LookAtLocation = TargetLocation + FVector(0.0f, 0.0f, GuessLookAtHeightOffset);

	constexpr int32 PlayersPerRow = 4;
	int32 ValidPlayerIndex = 0;
	for (APlayerController* Player : PlayerOrderList)
	{
		if (!IsValid(Player) || !IsValid(Player->GetPawn()))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameMode][GuessSpawn] 이동할 Pawn이 없습니다: %s"),
				*GetNameSafe(Player));
			continue;
		}

		const int32 Row = ValidPlayerIndex / PlayersPerRow;
		const int32 Column = ValidPlayerIndex % PlayersPerRow;
		const int32 PlayersBeforeThisRow = Row * PlayersPerRow;
		const int32 PlayersInThisRow = FMath::Min(
			PlayersPerRow, PlayerOrderList.Num() - PlayersBeforeThisRow);
		const float CenteredColumn = static_cast<float>(Column)
			- static_cast<float>(PlayersInThisRow - 1) * 0.5f;

		const FVector SpawnLocation = FormationAnchor
			+ RightDirection * (CenteredColumn * GuessPlayerSpacing)
			- ViewDirection * (static_cast<float>(Row) * GuessRowSpacing);
		const FRotator FullLookRotation = (LookAtLocation - SpawnLocation).Rotation();
		const FRotator PawnRotation(0.0f, FullLookRotation.Yaw, 0.0f);

		APawn* Pawn = Player->GetPawn();
		if (UPawnMovementComponent* MovementComponent = Pawn->GetMovementComponent())
		{
			MovementComponent->StopMovementImmediately();
		}

		Pawn->SetActorLocationAndRotation(
			SpawnLocation,
			PawnRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		// SetControlRotation alone only updates the server-side Controller. For remote
		// players, force the owning client camera/control rotation through the RPC.
		Player->SetControlRotation(FullLookRotation);
		Player->ClientSetRotation(FullLookRotation, true);
		Pawn->ForceNetUpdate();

		UE_LOG(LogTemp, Log,
			TEXT("[GameMode][GuessSpawn] %s -> Slot %d, Location=%s, TargetArea=%s"),
			*GetNameSafe(Player), ValidPlayerIndex, *SpawnLocation.ToCompactString(),
			*GetNameSafe(TargetArea));
		++ValidPlayerIndex;
	}
}

void ASIGameMode::EndGuessTurn()
{
	++CurrentWorkspaceIndex;
	StartNextGuessTurn();
}

void ASIGameMode::OnChatReceived(APlayerController* Sender, const FString& Message)
{
	if (!IsValid(Sender) || !PlayerOrderList.Contains(Sender))
	{
		return;
	}

	FString NormalizedMessage = Message.TrimStartAndEnd();
	if (NormalizedMessage.IsEmpty())
	{
		return;
	}
	NormalizedMessage.LeftInline(MaxChatLength);

	BroadcastChat(Sender, NormalizedMessage);
}

void ASIGameMode::OnAnswerSubmitted(APlayerController* Submitter, const FString& SubmittedAnswer)
{
	ASIGameState* SIState = GetGameState<ASIGameState>();
	if (!SIState || !IsValid(Submitter) || !PlayerOrderList.Contains(Submitter))
	{
		return;
	}

	FString NormalizedAnswer = SubmittedAnswer.TrimStartAndEnd();
	if (NormalizedAnswer.IsEmpty() || SIState->CurrentGamePhase != ESIGamePhase::GuessPhase)
	{
		return;
	}
	NormalizedAnswer.LeftInline(MaxChatLength);

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

	ASIPlayerState* SubmitterState = Submitter->GetPlayerState<ASIPlayerState>();
	if (!SubmitterState)
	{
		return;
	}

	FAnswerResultPayload Payload;
	Payload.Submitter = SubmitterState;
	Payload.SubmittedAnswer = NormalizedAnswer;

	const bool bMatchesAnswer = NormalizedAnswer.Equals(*CorrectAnswer, ESearchCase::IgnoreCase);
	if (!bMatchesAnswer)
	{
		SIState->Multicast_BroadcastAnswerResult(Payload);
		return;
	}

	// The workspace owner and players who already answered correctly cannot earn points.
	if (Submitter == WorkspaceOwner || CorrectPlayersThisTurn.Contains(Submitter))
	{
		return;
	}

	const int32 ScoreIndex = FMath::Min(
		CorrectPlayersThisTurn.Num(), CorrectAnswerScores.Num() - 1);
	const int32 ScoreToEarn = CorrectAnswerScores.IsValidIndex(ScoreIndex)
		? FMath::Max(CorrectAnswerScores[ScoreIndex], 0)
		: 0;
	SubmitterState->AddScore(ScoreToEarn);
	CorrectPlayersThisTurn.Add(Submitter);

	// Do not include the correct answer in data multicast to every client.
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
		ProcessServerTravel(
			TEXT("/Game/Shape_It/Level/Test_Lobby?listen?game=/Script/TeamProject.SILobbyGameMode"));
	}, 5.0f, false);
}
