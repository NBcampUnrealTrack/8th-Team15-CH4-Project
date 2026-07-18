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

void ASILobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ASIGameState* SIState = GetGameState<ASIGameState>())
	{
		SIState->SetGamePhase(ESIGamePhase::LobbyPhase);
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
	SIState->Mulitcast_GameStartSound();

	FTimerHandle TravelTimerHandle;
	float StartDelay = 3.0f;

	GetWorldTimerManager().SetTimer(TravelTimerHandle, [this, SIState]()
		{
			GetWorld()->ServerTravel(
				TEXT("/Game/Shape_It/Level/MainLevel?listen?game=/Game/Shape_It/Game/BP_GameMode.BP_GameMode_C"));
		}, StartDelay, false);
}

