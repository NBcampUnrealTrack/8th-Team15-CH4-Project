#include "SIGameMode.h"

#include "Character/SICharacter.h"
#include "Engine/DataTable.h"
#include "EngineUtils.h"				// TActorIterator
#include "GameFramework/PlayerStart.h"
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

AActor* ASIGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1순위: 엔진 기본 선택 (비어 있는 PlayerStart → 비켜설 자리가 있는 PlayerStart)
	if (AActor* ChosenStart = Super::ChoosePlayerStart_Implementation(Player))
	{
		return ChosenStart;
	}

	// 2순위: 자리가 겹치더라도 아무 PlayerStart나 쓴다.
	// 여기까지 왔다는 건 "쓸 수 있는 자리가 없다"는 뜻인데, 그대로 nullptr을 돌려주면
	// 엔진이 폰 생성을 포기해 조작 불가능한 관전 카메라만 남는다(플레이 자체가 불가).
	// 겹쳐 스폰되는 건 잠깐이고 곧 작업공간으로 재배치되므로 이쪽이 훨씬 낫다.
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameMode][Spawn] 비어 있는 PlayerStart가 없어 %s를 겹쳐서 스폰합니다. "
				"MainLevel의 PlayerStart 개수를 최대 인원(%d)만큼 늘리는 것을 권장합니다."),
			*GetNameSafe(Player), MaxPlayers);
		return *It;
	}

	UE_LOG(LogTemp, Error,
		TEXT("[GameMode][Spawn] 레벨에 PlayerStart가 하나도 없습니다. %s의 폰을 스폰할 수 없습니다."),
		*GetNameSafe(Player));
	return nullptr;
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
		&& CurrentSIState->CurrentGamePhase == ESIGamePhase::TurnPhase;

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

			// 호스트 이탈은 알리지 않는다 — 리슨 서버라 방이 통째로 사라진다(로비와 같은 규칙).
			// GetNetConnection()이 아니라 bIsHost로 판단하는 이유는 SILobbyGameMode::Logout 주석 참고
			// (Logout 시점엔 참가자도 커넥션이 이미 null이라 전원이 호스트로 오인된다).
			// 아래의 새 호스트 승계보다 먼저 읽어야 나가는 본인의 값을 본다.
			// 로비 복귀 트래블로 인한 Logout도 퇴장이 아니므로 안내하지 않는다
			// (ASILobbyGameMode::Logout의 게임 시작 트래블 처리와 같은 이유).
			const ASIPlayerState* ExitingSIState = Cast<ASIPlayerState>(ExitingPlayerState);
			if ((!IsValid(ExitingSIState) || !ExitingSIState->bIsHost) && !bTravelRequested)
			{
				SIState->AnnouncePlayerLeft(ExitingPlayerState);
			}
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
		bTurnTransitionPending = true;
		GetWorldTimerManager().SetTimerForNextTick(this, &ASIGameMode::AdvanceFromDisconnectedBuilder);
	}
	else if (CurrentSIState && CurrentSIState->CurrentGamePhase == ESIGamePhase::TurnPhase
		&& PlayerOrderList.IsValidIndex(CurrentWorkspaceIndex)
		&& HaveAllEligibleGuessersAnswered(PlayerOrderList[CurrentWorkspaceIndex]))
	{
		ScheduleTurnEndAfterAllCorrect();
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

	if (SIState->CurrentGamePhase == ESIGamePhase::TurnPhase
		|| SIState->CurrentGamePhase == ESIGamePhase::BuildPhase
		|| SIState->CurrentGamePhase == ESIGamePhase::GuessPhase)
	{
		return;
	}
	
	// 호스트가 로비에서 확정한 턴 시간과 최대 도형 개수를 적용합니다.
	ApplyHostMatchSettings();   

	for (APlayerController* Player : PlayerOrderList)
	{
		if (IsValid(Player))
		{
			if (ASICharacter* Character = Cast<ASICharacter>(Player->GetPawn()))
			{
				Character->SetMaxPlacedShapeCount(MaxPlacedShapeCount);
			}
		}
	}

	if (!AssignWordsToPlayers())
	{
		return;
	}

	PlayerWorkspaceAreas.Empty();
	AssignPlayerWorkspaces();

	for (APlayerController* Player : PlayerOrderList)
	{
		if (!PlayerWorkspaceAreas.Contains(Player))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameMode][Workspace] %s의 작업공간이 등록되지 않았습니다. "
					"BP AssignPlayerWorkspaces에서 RegisterPlayerWorkspace를 호출해야 합니다."),
				*GetNameSafe(Player));
		}
	}

	CurrentWorkspaceIndex = 0;
	CorrectPlayersThisTurn.Empty();
	SIState->CurrentRound = 0;
	SIState->TotalRounds = PlayerOrderList.Num();
	SIState->SetCurrentWorkspaceOwner(nullptr);
	StartNextTurn();

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
	MaxPlacedShapeCount = FMath::Clamp(
		HostParams.MaxPlacedShapeCount,
		SIRoomSettingLimits::MinPlacedShapeCount,
		SIRoomSettingLimits::MaxPlacedShapeCount);

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Match settings: Build=%.0fs, MaxShapes=%d"),
		BuildTimeLimit, MaxPlacedShapeCount);
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

void ASIGameMode::StartNextTurn()
{
	GetWorldTimerManager().ClearTimer(TurnTransitionTimerHandle);
	bTurnTransitionPending = false;

	while (PlayerOrderList.IsValidIndex(CurrentWorkspaceIndex))
	{
		APlayerController* Builder = PlayerOrderList[CurrentWorkspaceIndex];
		if (IsValid(Builder) && PlayerAssignedWords.Contains(Builder)
			&& PlayerWorkspaceAreas.Contains(Builder))
		{
			break;
		}
		++CurrentWorkspaceIndex;
	}

	if (!PlayerOrderList.IsValidIndex(CurrentWorkspaceIndex))
	{
		EndMatch();
		return;
	}

	APlayerController* Builder = PlayerOrderList[CurrentWorkspaceIndex];
	CorrectPlayersThisTurn.Empty();

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		SIState->CurrentRound = CurrentWorkspaceIndex + 1;
		SIState->TotalRounds = PlayerOrderList.Num();
		SIState->SetCurrentWorkspaceOwner(Builder->PlayerState);
		SIState->SetRemainingTime(FMath::CeilToInt(BuildTimeLimit));
		SIState->SetGamePhase(ESIGamePhase::TurnPhase);
	}

	MoveBuilderToWorkspace(Builder);
	SpawnGuessersToTargetWorkspace(Builder);
	SendTurnRoles(Builder);

	GetWorldTimerManager().SetTimer(
		GameTimerHandle, this, &ASIGameMode::EndCurrentTurn, BuildTimeLimit, false);
}

bool ASIGameMode::MoveBuilderToWorkspace(APlayerController* Builder)
{
	if (!HasAuthority() || !IsValid(Builder))
	{
		return false;
	}

	const TObjectPtr<AActor>* WorkspacePtr = PlayerWorkspaceAreas.Find(Builder);
	AActor* Workspace = WorkspacePtr ? WorkspacePtr->Get() : nullptr;
	APawn* Pawn = Builder->GetPawn();
	if (!IsValid(Workspace) || !IsValid(Pawn))
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode][BuilderSpawn] Missing pawn or workspace: %s"),
			*GetNameSafe(Builder));
		return false;
	}

	if (UPawnMovementComponent* MovementComponent = Pawn->GetMovementComponent())
	{
		MovementComponent->StopMovementImmediately();
	}

	const FVector Location = Workspace->GetActorLocation();
	const FRotator Rotation = Workspace->GetActorRotation();
	Pawn->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	Builder->SetControlRotation(Rotation);
	Builder->ClientSetRotation(Rotation, true);
	Pawn->ForceNetUpdate();
	return true;
}

void ASIGameMode::SpawnGuessersToTargetWorkspace(APlayerController* TargetOwner)
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

	TArray<APlayerController*> Guessers;
	for (APlayerController* Player : PlayerOrderList)
	{
		if (Player != TargetOwner && IsValid(Player) && IsValid(Player->GetPawn()))
		{
			Guessers.Add(Player);
		}
	}

	constexpr int32 PlayersPerRow = 4;
	for (int32 GuesserIndex = 0; GuesserIndex < Guessers.Num(); ++GuesserIndex)
	{
		APlayerController* Player = Guessers[GuesserIndex];
		const int32 Row = GuesserIndex / PlayersPerRow;
		const int32 Column = GuesserIndex % PlayersPerRow;
		const int32 PlayersBeforeThisRow = Row * PlayersPerRow;
		const int32 PlayersInThisRow = FMath::Min(
			PlayersPerRow, Guessers.Num() - PlayersBeforeThisRow);
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
			*GetNameSafe(Player), GuesserIndex, *SpawnLocation.ToCompactString(),
			*GetNameSafe(TargetArea));
	}
}

void ASIGameMode::EndCurrentTurn()
{
	if (bTurnTransitionPending)
	{
		return;
	}

	bTurnTransitionPending = true;
	GetWorldTimerManager().ClearTimer(GameTimerHandle);

	if (PlayerOrderList.IsValidIndex(CurrentWorkspaceIndex))
	{
		if (APlayerController* Builder = PlayerOrderList[CurrentWorkspaceIndex])
		{
			if (ASICharacter* Character = Cast<ASICharacter>(Builder->GetPawn()))
			{
				Character->RestoreActiveShapeEditForPhaseChange();
			}
		}
	}

	++CurrentWorkspaceIndex;
	GetWorldTimerManager().SetTimerForNextTick(this, &ASIGameMode::StartNextTurn);
}

void ASIGameMode::ScheduleTurnEndAfterAllCorrect()
{
	if (bTurnTransitionPending)
	{
		return;
	}

	bTurnTransitionPending = true;
	GetWorldTimerManager().ClearTimer(GameTimerHandle);

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		SIState->SetRemainingTime(0);
	}

	if (CorrectAnswerTransitionDelay <= 0.0f)
	{
		CompleteDelayedTurnEnd();
		return;
	}

	GetWorldTimerManager().SetTimer(
		TurnTransitionTimerHandle,
		this,
		&ASIGameMode::CompleteDelayedTurnEnd,
		CorrectAnswerTransitionDelay,
		false);
}

void ASIGameMode::CompleteDelayedTurnEnd()
{
	bTurnTransitionPending = false;
	EndCurrentTurn();
}

void ASIGameMode::AdvanceFromDisconnectedBuilder()
{
	bTurnTransitionPending = false;
	StartNextTurn();
}

int32 ASIGameMode::GetEligibleGuesserCount(APlayerController* Builder) const
{
	int32 Count = 0;
	for (APlayerController* Player : PlayerOrderList)
	{
		if (IsValid(Player) && Player != Builder)
		{
			++Count;
		}
	}
	return Count;
}

bool ASIGameMode::HaveAllEligibleGuessersAnswered(APlayerController* Builder) const
{
	const int32 EligibleCount = GetEligibleGuesserCount(Builder);
	return EligibleCount > 0 && CorrectPlayersThisTurn.Num() >= EligibleCount;
}

void ASIGameMode::SendTurnRoles(APlayerController* Builder)
{
	const FSIAssignedKeyword* Keyword = PlayerAssignedWords.Find(Builder);
	for (APlayerController* Player : PlayerOrderList)
	{
		if (ASIPlayerController* SIPlayer = Cast<ASIPlayerController>(Player))
		{
			SIPlayer->Client_ReceiveSecretWord(
				Player == Builder && Keyword ? Keyword->Keyword : FString());
		}
	}
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

void ASIGameMode::OnAnswerSubmitted(
	APlayerController* Submitter, const FString& SubmittedAnswer, const int32 SubmittedRound)
{
	ASIGameState* SIState = GetGameState<ASIGameState>();
	if (!SIState || !IsValid(Submitter) || !PlayerOrderList.Contains(Submitter))
	{
		return;
	}

	FString NormalizedAnswer = SubmittedAnswer.TrimStartAndEnd();
	if (NormalizedAnswer.IsEmpty()
		|| SIState->CurrentGamePhase != ESIGamePhase::TurnPhase
		|| SIState->CurrentRound != SubmittedRound
		|| bTurnTransitionPending)
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

	if (HaveAllEligibleGuessersAnswered(WorkspaceOwner))
	{
		ScheduleTurnEndAfterAllCorrect();
	}
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
	GetWorldTimerManager().ClearTimer(TurnTransitionTimerHandle);

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		SIState->SetRemainingTime(0);
		SIState->SetGamePhase(ESIGamePhase::ResultPhase);
		SIState->SetCurrentWorkspaceOwner(nullptr);
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

	// 이 뒤로 쏟아지는 Logout은 트래블 때문이지 퇴장이 아니다 (Logout의 안내 억제용).
	bTravelRequested = true;

	// 도착한 로비가 "이 사람들은 새로 들어온 게 아니라 돌아온 것"을 알 수 있게 명단을 넘긴다.
	// GameInstance는 논심리스 트래블을 넘어 살아남으므로 이 경로로만 전달이 가능하다.
	if (USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>())
	{
		if (const ASIGameState* SIState = GetGameState<ASIGameState>())
		{
			SIInstance->SealReturningRoster(SIState->PlayerArray);
		}
	}

	ProcessServerTravel(
		TEXT("/Game/Shape_It/Level/Test_Lobby?listen?game=/Script/TeamProject.SILobbyGameMode"));
}
