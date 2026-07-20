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
	// 로비 채팅도 같은 한도를 써야 하므로 실제 값은 SITypes.h의 SIChatLimits에 있다
	constexpr int32 MaxChatLength = SIChatLimits::MaxMessageLength;

	/** 프로퍼티 이름이 접두어와 맞는지 검사한다.
		BP에서 만든 구조체의 프로퍼티는 실제 이름이 "Keyword_2_A1B2C3..." 처럼 맹글링되므로
		정확히 일치(C++ 구조체)와 접두어 일치(BP 구조체)를 둘 다 본다. */
	bool MatchesPropertyName(const FProperty* Property, const TCHAR* BaseName)
	{
		if (!Property)
		{
			return false;
		}

		const FString PropertyName = Property->GetName();
		if (PropertyName.Equals(BaseName, ESearchCase::IgnoreCase)
			|| PropertyName.StartsWith(FString(BaseName) + TEXT("_"), ESearchCase::IgnoreCase))
		{
			return true;
		}

#if WITH_EDITORONLY_DATA
		// FField::GetDisplayNameText()는 WITH_EDITORONLY_DATA 안에 선언된 에디터 전용 API라
		// 패키징(Game 타겟)에선 함수 자체가 존재하지 않는다. 감싸지 않으면 에디터 빌드만
		// 통과하고 패키징에서 C2039로 깨진다.
		// 빠져도 안전한 이유: 표시 이름이 "Keyword"인 BP 프로퍼티는 실제 이름도 "Keyword_..."로
		// 생성되어 위 StartsWith에 이미 걸린다. 즉 이 검사는 보조 수단이고 매칭 결과는 같다.
		return Property->GetDisplayNameText().ToString().Equals(BaseName, ESearchCase::IgnoreCase);
#else
		return false;
#endif
	}

	bool IsKeywordProperty(const FProperty* Property)
	{
		return MatchesPropertyName(Property, TEXT("Keyword"));
	}

	bool IsLevelProperty(const FProperty* Property)
	{
		return MatchesPropertyName(Property, TEXT("Level"));
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

	bool ReadLevelValue(const FProperty* Property, const uint8* RowData, int32& OutLevel)
	{
		if (!Property || !RowData)
		{
			return false;
		}

		FString ExportedValue;
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(RowData);
		Property->ExportTextItem_Direct(
			ExportedValue, ValuePtr, nullptr, nullptr, PPF_None);
		ExportedValue = ExportedValue.TrimStartAndEnd().TrimQuotes();
		return LexTryParseString(OutLevel, *ExportedValue);
	}
}

ASIGameMode::ASIGameMode()
{
	CreatorFirstCorrectScoresByLevel.Add(1, 1);
	CreatorFirstCorrectScoresByLevel.Add(2, 2);
	CreatorFirstCorrectScoresByLevel.Add(3, 3);

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

	// 인게임 맵에 도착 = 게임중. 방 목록에 그렇게 표시된다.
	if (UGameInstance* GameInstanceRef = GetGameInstance())
	{
		if (USISessionSubsystem* Subsystem = GameInstanceRef->GetSubsystem<USISessionSubsystem>())
		{
			Subsystem->SetSessionInProgress(true);
		}
	}
}

void ASIGameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// Super가 이미 거절 사유를 채웠으면 그대로 존중
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	const USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>();

	// 판단 근거가 없으면 통과시킨다 (SILobbyGameMode::PreLogin과 같은 "개발 편의" 방침).
	// 명단이 봉인되지 않았다 = 로비를 거치지 않고 이 맵이 직접 열렸다(에디터 단독 실행 등).
	if (!IsValid(SIInstance) || !SIInstance->IsMatchRosterSealed())
	{
		return;
	}

	// 로비에서 함께 출발한 인원 — travel로 재접속하는 정상 경로
	if (SIInstance->IsInMatchRoster(UniqueId))
	{
		return;
	}

	ErrorMessage = SISessionErrors::GameInProgress;

	UE_LOG(LogTemp, Warning,
		TEXT("[GameMode] PreLogin 거절: 이미 시작된 매치에 중도 참여 시도 (%s)"), *Address);
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

		// 입장이 확정됐으니 이 사람 몫의 입장권을 회수한다.
		// 이후 같은 사람이 다시 접속을 시도하면(게임 중 튕김 등) 명단에 없어 거절된다 —
		// 워크스페이스 배정이 끝난 뒤라 들여봐야 폰 없이 카메라만 남기 때문.
		if (USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>())
		{
			SIInstance->ConsumeMatchRosterEntry(PlayerState->GetUniqueId());
		}

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
	const FProperty* LevelProperty = nullptr;
	for (TFieldIterator<FProperty> PropertyIt(KeywordDataTable->GetRowStruct()); PropertyIt; ++PropertyIt)
	{
		if (IsKeywordProperty(*PropertyIt))
		{
			KeywordProperty = *PropertyIt;
		}
		else if (IsLevelProperty(*PropertyIt))
		{
			LevelProperty = *PropertyIt;
		}
	}

	if (!KeywordProperty || !LevelProperty)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GameMode] Keyword DataTable Row에서 Keyword 또는 Level 필드를 찾지 못했습니다."));
		return false;
	}

	TArray<FSIAssignedKeyword> CandidateWords;
	TSet<FString> UniqueWords;
	for (const TPair<FName, uint8*>& RowPair : KeywordDataTable->GetRowMap())
	{
		FString Word = ReadKeywordValue(KeywordProperty, RowPair.Value).TrimStartAndEnd();
		int32 Level = 0;
		if (!ReadLevelValue(LevelProperty, RowPair.Value, Level) || Level <= 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameMode] Keyword DataTable의 '%s' 행에 유효한 Level이 없습니다."),
				*RowPair.Key.ToString());
			continue;
		}

		const FString ComparisonKey = Word.ToLower();
		if (!Word.IsEmpty() && !UniqueWords.Contains(ComparisonKey))
		{
			UniqueWords.Add(ComparisonKey);
			FSIAssignedKeyword Candidate;
			Candidate.Keyword = MoveTemp(Word);
			Candidate.Level = Level;
			CandidateWords.Add(MoveTemp(Candidate));
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
		const FSIAssignedKeyword* AssignedKeyword = PlayerAssignedWords.Find(Player);
		if (AssignedKeyword)
		{
			if (ASIPlayerController* SIPlayer = Cast<ASIPlayerController>(Player))
			{
				SIPlayer->Client_ReceiveSecretWord(AssignedKeyword->Keyword);
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
		BuildTimeLimit = FMath::Clamp(HostParams.BuildTime,
			SIRoomSettingLimits::MinBuildTime, SIRoomSettingLimits::MaxBuildTime);
	}
	if (HostParams.GuessTime > 0.0f)
	{
		GuessTimeLimit = FMath::Clamp(HostParams.GuessTime,
			SIRoomSettingLimits::MinGuessTime, SIRoomSettingLimits::MaxGuessTime);
	}

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Match settings: Build=%.0fs, Guess=%.0fs"),
		BuildTimeLimit, GuessTimeLimit);
}

int32 ASIGameMode::GetCreatorScoreForCorrectAnswer(
	const int32 KeywordLevel, const bool bIsFirstCorrect) const
{
	if (!bIsFirstCorrect)
	{
		return FMath::Max(CreatorAdditionalCorrectScore, 0);
	}

	const int32* LevelScore = CreatorFirstCorrectScoresByLevel.Find(KeywordLevel);
	if (!LevelScore)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameMode] Level %d의 최초 정답 제작자 점수가 설정되지 않았습니다."),
			KeywordLevel);
		return 0;
	}

	return FMath::Max(*LevelScore, 0);
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
	const FSIAssignedKeyword* AssignedKeyword = PlayerAssignedWords.Find(WorkspaceOwner);
	if (!IsValid(WorkspaceOwner) || !AssignedKeyword)
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

	const bool bMatchesAnswer = NormalizedAnswer.Equals(
		AssignedKeyword->Keyword, ESearchCase::IgnoreCase);
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

	const bool bIsFirstCorrect = CorrectPlayersThisTurn.IsEmpty();
	const int32 ScoreIndex = FMath::Min(
		CorrectPlayersThisTurn.Num(), CorrectAnswerScores.Num() - 1);
	const int32 ScoreToEarn = CorrectAnswerScores.IsValidIndex(ScoreIndex)
		? FMath::Max(CorrectAnswerScores[ScoreIndex], 0)
		: 0;
	SubmitterState->AddScore(ScoreToEarn);
	CorrectPlayersThisTurn.Add(Submitter);

	if (ASIPlayerState* CreatorState = WorkspaceOwner->GetPlayerState<ASIPlayerState>())
	{
		const int32 CreatorScore = GetCreatorScoreForCorrectAnswer(
			AssignedKeyword->Level, bIsFirstCorrect);
		if (CreatorScore > 0)
		{
			CreatorState->AddScore(CreatorScore);
		}
	}

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

	// 점수판을 이만큼 보여준 뒤 로비로 돌아간다
	constexpr float ScoreBoardDuration = 5.0f;

	// 복귀 직전에 로딩 화면으로 점수판을 덮는다.
	// 트래블과 같은 프레임에 쏘면 클라가 멀티캐스트보다 맵 이동을 먼저 처리할 수 있어 여유를 둔다.
	constexpr float LoadingScreenLeadTime = 0.5f;

	GetWorldTimerManager().SetTimer(ResultLoadingScreenTimerHandle, [this]()
	{
		if (ASIGameState* SIState = GetGameState<ASIGameState>())
		{
			SIState->Multicast_ShowLoadingScreen();
		}
	}, ScoreBoardDuration - LoadingScreenLeadTime, false);

	GetWorldTimerManager().SetTimer(
		ReturnToLobbyTimerHandle, this, &ASIGameMode::ReturnToLobby, ScoreBoardDuration, false);
}

void ASIGameMode::ReturnToLobby()
{
	GetWorldTimerManager().ClearTimer(ReturnToLobbyTimerHandle);
	GetWorldTimerManager().ClearTimer(ResultLoadingScreenTimerHandle);

	ProcessServerTravel(
		TEXT("/Game/Shape_It/Level/Test_Lobby?listen?game=/Script/TeamProject.SILobbyGameMode"));
}
