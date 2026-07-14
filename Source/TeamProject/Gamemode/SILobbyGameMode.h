#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SILobbyGameMode.generated.h"

class ASIPlayerState;

/**
 * 로비에서 방장/참가자를 관리하고 모든 플레이어를 MainLevel로 이동시키는 전용 GameMode입니다.
 */
UCLASS()
class TEAMPROJECT_API ASILobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASILobbyGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void RequestStartGame(ASIPlayerState* RequestingPlayerState);

protected:
	virtual void BeginPlay() override;

private:
	bool bTravelRequested = false;

	void AssignHostIfNeeded();
};
