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
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** 스폰 위치 선택은 엔진 기본 동작(PlayerStart 중 랜덤)을 그대로 쓴다.
		여기서 하는 일은 "누가 어느 PlayerStart의 어디에 떴는지"를 남기는 것뿐 —
		'로비와 엉뚱한 곳에 스폰된다'는 증상이 재현됐을 때 레벨 배치 문제인지
		PlayerStart를 못 찾은 것인지 로그만으로 구분하기 위해서다. */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void RequestStartGame(ASIPlayerState* RequestingPlayerState);

	/** 호스트가 로비에서 방 설정을 바꿨을 때의 서버측 처리 지점.
		권한 검증 → 허용 범위로 clamp → 보관값(세션) 갱신 → GameState 복제 순으로 진행한다. */
	void UpdateRoomSettings(ASIPlayerState* RequestingPlayerState, const FString& RoomTitle,
		const FString& Password, float BuildTime, int32 MaxPlacedShapeCount);

protected:
	virtual void BeginPlay() override;

private:
	bool bTravelRequested = false;

	void AssignHostIfNeeded();
};
