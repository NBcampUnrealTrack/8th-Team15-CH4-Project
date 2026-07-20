#include "SILobbyGameMode.h"

#include "GameInstance/SIGameInstance.h"
#include "GameInstance/SISessionSubsystem.h"
#include "GameState/SIGameState.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/SIPlayerState.h"
#include "UObject/ConstructorHelpers.h"

ASILobbyGameMode::ASILobbyGameMode()
{		
	static ConstructorHelpers::FClassFinder<AGameStateBase> GameStateFinder(
		TEXT("/Game/Shape_It/Game/BP_GameState"));
	if (GameStateFinder.Succeeded())
	{
		GameStateClass = GameStateFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerState> PlayerStateFinder(
		TEXT("/Game/Shape_It/Game/BP_PlayerState"));
	if (PlayerStateFinder.Succeeded())
	{
		PlayerStateClass = PlayerStateFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerFinder(
		TEXT("/Game/Shape_It/PlayerController/BP_PlayerController"));
	if (PlayerControllerFinder.Succeeded())
	{
		PlayerControllerClass = PlayerControllerFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<APawn> DefaultPawnFinder(
		TEXT("/Game/Shape_It/Character/Blueprint/BP_PlayerCharacter"));
	if (DefaultPawnFinder.Succeeded())
	{
		DefaultPawnClass = DefaultPawnFinder.Class;
	}
}

void ASILobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	
	// Super가 이미 거절 사유를 채웠으면 그대로 존중
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}
	
	// 호스트가 보관 중인 방 설정 조회 (서버에서만 실행되므로 호스트의 서브시스템)
	const UGameInstance* GameIstance = GetGameInstance();
	
	// 게임 인스턴스 없으면 검증 불가 — 일단 통과 (개발 편의)
	if (IsValid(GameIstance) == false)
	{
		return;
	}
	
	const USISessionSubsystem* Subsystem = GameIstance->GetSubsystem<USISessionSubsystem>();
	
	// 서브시스템 없으면 검증 불가 — 일단 통과 (개발 편의)
	if (IsValid(Subsystem) == false)
	{
		return;   
	}

	const FSICreateSessionParams& HostParams = Subsystem->GetHostSessionParams();
	
	// 공개 방 — 검증 없음
	if (HostParams.Password.IsEmpty())
	{
		return;		// 비밀번호 없는 방 — 검증 없음
	}

	// 클라이언트가 URL에 실어 보낸 값 추출
	const FString ClientPassword = UGameplayStatics::ParseOption(Options, TEXT("Password"));

	if (ClientPassword != HostParams.Password)
	{
		// 채우는 순간 접속 거절
		ErrorMessage = SISessionErrors::WrongPassword;   
		
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] PreLogin rejected: wrong password from %s"), *Address);
	}
}

AActor* ASILobbyGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	AActor* ChosenStart = Super::ChoosePlayerStart_Implementation(Player);

	if (!IsValid(ChosenStart))
	{
		// 여기 걸리면 레벨에 쓸 수 있는 PlayerStart가 하나도 없다는 뜻.
		// 엔진은 이 경우 폰을 아예 스폰하지 않는다(관전 카메라만 남는다).
		UE_LOG(LogTemp, Error,
			TEXT("[Lobby][Spawn] %s: 사용 가능한 PlayerStart를 찾지 못했습니다. "
				"Test_Lobby의 PlayerStart 배치를 확인하세요."),
			*GetNameSafe(Player));
		return ChosenStart;
	}

	UE_LOG(LogTemp, Log, TEXT("[Lobby][Spawn] %s -> %s %s"),
		*GetNameSafe(Player),
		*ChosenStart->GetActorNameOrLabel(),
		*ChosenStart->GetActorLocation().ToCompactString());

	return ChosenStart;
}

void ASILobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	ASIGameState* SIState = GetGameState<ASIGameState>();
	if (!SIState)
	{
		return;
	}

	SIState->SetGamePhase(ESIGamePhase::LobbyPhase);

	// 로비에 있다 = 매치가 진행 중이 아니다. 봉인해둔 참가자 명단을 풀어 다시 입장을 받는다.
	// 첫 입장이든 게임을 마치고 돌아온 것이든 동일하게 동작한다(멱등).
	if (USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>())
	{
		SIInstance->ClearMatchRoster();
	}

	// 호스트 GameInstance가 보관 중인 방 설정을 GameState로 옮겨 클라이언트 UI까지 도달시킵니다.
	if (const UGameInstance* GameInstanceRef = GetGameInstance())
	{
		if (USISessionSubsystem* Subsystem = GameInstanceRef->GetSubsystem<USISessionSubsystem>())
		{
			const FSICreateSessionParams& HostParams = Subsystem->GetHostSessionParams();
			SIState->SetLobbyRoomInfo(
				HostParams.RoomTitle, HostParams.MaxPlayers, HostParams.BuildTime, HostParams.GuessTime);

			// 로비에 있다 = 대기중. 게임이 끝나고 돌아온 경우까지 여기서 함께 처리된다.
			Subsystem->SetSessionInProgress(false);
		}
	}
}

void ASILobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (IsValid(NewPlayer) && NewPlayer->GetNetConnection() == nullptr)
	{
		if (ASIPlayerState* HostState = NewPlayer->GetPlayerState<ASIPlayerState>())
		{
			HostState->SetIsHost(true);
		}
	}

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		if (APlayerState* JoinedPlayerState = IsValid(NewPlayer) ? NewPlayer->PlayerState : nullptr)
		{
			SIState->Multicast_BroadcastPlayerJoined(JoinedPlayerState);
		}
	}
}

void ASILobbyGameMode::Logout(AController* Exiting)
{
	APlayerState* ExitingPlayerState = IsValid(Exiting) ? Exiting->PlayerState : nullptr;

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		if (IsValid(ExitingPlayerState))
		{
			SIState->Multicast_BroadcastPlayerLeft(ExitingPlayerState);
		}
	}

	Super::Logout(Exiting);
	AssignHostIfNeeded();
}

void ASILobbyGameMode::AssignHostIfNeeded()
{
	ASIGameState* SIState = GetGameState<ASIGameState>();
	if (!SIState)
	{
		return;
	}

	ASIPlayerState* FirstPlayerState = nullptr;
	bool bHasHost = false;
	for (APlayerState* PlayerState : SIState->PlayerArray)
	{
		if (ASIPlayerState* SIPlayerState = Cast<ASIPlayerState>(PlayerState))
		{
			FirstPlayerState = FirstPlayerState ? FirstPlayerState : SIPlayerState;
			bHasHost |= SIPlayerState->bIsHost;
		}
	}

	if (!bHasHost && FirstPlayerState)
	{
		FirstPlayerState->SetIsHost(true);
	}
}

void ASILobbyGameMode::UpdateRoomSettings(ASIPlayerState* RequestingPlayerState, const FString& RoomTitle,
	const FString& Password, const float BuildTime, const float GuessTime)
{
	// 이미 게임 시작이 예약됐으면 설정 변경을 받지 않는다 (travel 직전 값이 흔들리는 것 방지)
	if (bTravelRequested || !IsValid(RequestingPlayerState) || !RequestingPlayerState->bIsHost)
	{
		return;
	}

	UGameInstance* GameInstanceRef = GetGameInstance();
	USISessionSubsystem* Subsystem =
		GameInstanceRef ? GameInstanceRef->GetSubsystem<USISessionSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	// 기존 값에서 출발해 UI가 보낸 항목만 덮어쓴다 (MaxPlayers처럼 UI에 없는 필드 보존)
	FSICreateSessionParams NewParams = Subsystem->GetHostSessionParams();

	// 방 제목이 비면 기존 제목을 유지한다 — 목록에 이름 없는 방이 뜨는 걸 막는다
	if (!RoomTitle.IsEmpty())
	{
		NewParams.RoomTitle = RoomTitle;
	}

	NewParams.Password = Password;

	// 0 이하 = "미지정" 센티널이라 그대로 통과시킨다. 지정된 값만 상식 범위로 자른다.
	NewParams.BuildTime = BuildTime > 0.0f
		? FMath::Clamp(BuildTime, SIRoomSettingLimits::MinBuildTime, SIRoomSettingLimits::MaxBuildTime)
		: 0.0f;

	NewParams.GuessTime = GuessTime > 0.0f
		? FMath::Clamp(GuessTime, SIRoomSettingLimits::MinGuessTime, SIRoomSettingLimits::MaxGuessTime)
		: 0.0f;

	Subsystem->UpdateHostSessionParams(NewParams);

	// 복제해서 참가자 UI까지 반영 (비밀번호는 복제하지 않는다)
	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		SIState->SetLobbyRoomInfo(
			NewParams.RoomTitle, NewParams.MaxPlayers, NewParams.BuildTime, NewParams.GuessTime);
	}
}

void ASILobbyGameMode::RequestStartGame(ASIPlayerState* RequestingPlayerState)
{
	if (bTravelRequested || !IsValid(RequestingPlayerState) || !RequestingPlayerState->bIsHost)
	{
		return;
	}

	ASIGameState* SIState = GetGameState<ASIGameState>();
	USIGameInstance* SIInstance = GetGameInstance<USIGameInstance>();
	if (!SIState || !SIInstance || SIState->PlayerArray.IsEmpty())
	{
		return;
	}

	bTravelRequested = true;
	SIInstance->PreparePendingMatch(SIState->PlayerArray.Num());

	// 지금 로비에 있는 인원만 이번 매치의 참가자다. 이 명단이 인게임 PreLogin의 통과 기준이 된다.
	// (여기서 봉인해야 travel 도중/이후에 들어오려는 접속을 걸러낼 수 있다)
	SIInstance->SealMatchRoster(SIState->PlayerArray);
	SIState->Mulitcast_GameStartSound();

	// 버튼을 누르는 즉시 로딩 화면으로 덮는다.
	// 트래블 직전에 띄우면 반응이 없는 대기 구간이 생기고, 이 화면은 조작 가이드를 담고 있어
	// 시작 사운드가 흐르는 동안 읽히는 편이 낫다.
	SIState->Multicast_ShowLoadingScreen();

	FTimerHandle TravelTimerHandle;
	float StartDelay = 3.0f;

	GetWorldTimerManager().SetTimer(TravelTimerHandle, [this, SIState]()
		{
			GetWorld()->ServerTravel(
				TEXT("/Game/Shape_It/Level/MainLevel?listen?game=/Game/Shape_It/Game/BP_GameMode.BP_GameMode_C"));
		}, StartDelay, false);
}

