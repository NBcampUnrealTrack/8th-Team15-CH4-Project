#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SIPlayerState.generated.h"

// UI 블루프린트에 점수 변경을 즉각적으로 알려줄 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreUpdatedSignature, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostStatusChanged, bool, bIsHost);

/**
 * 플레이어 개인의 점수 및 로비 상태를 관리하고 클라이언트 UI와 동기화하는 클래스입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASIPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ==========================================
	// [점수 관리]
	// ==========================================

	// 현재 게임에서 획득한 점수
	UPROPERTY(ReplicatedUsing = OnRep_CurrentScore, Transient, BlueprintReadOnly, Category = "GameData|Score")
	int32 CurrentScore = 0;

	// 서버에서 점수가 변경되었을 때 클라이언트에서 실행되는 함수
	UFUNCTION()
	void OnRep_CurrentScore();

	// UI에서 점수 텍스트를 업데이트할 때 이벤트를 받을 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnScoreUpdatedSignature OnScoreUpdated;

	// 정답을 맞췄을 때 서버에서 점수를 추가하기 위해 호출하는 함수
	UFUNCTION(BlueprintCallable, Category = "GameData|Score")
	void AddScore(int32 Amount);

	// ==========================================
	// [로비 및 게임 시작 관리]
	// ==========================================

	// 이 플레이어가 방장(호스트)인지 여부 
	UPROPERTY(ReplicatedUsing = OnRep_IsHost, BlueprintReadWrite, Category = "Lobby")
	bool bIsHost;
	
	UFUNCTION()
	void OnRep_IsHost();
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHostStatusChanged OnHostStatusChanged;
	
	void SetIsHost(bool bNewIsHost);
	
	// 호스트 UI에서 '게임 시작' 버튼을 눌렀을 때 서버에 게임 루프 시작을 요청하는 RPC 함수
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Lobby")
	void Server_RequestStartGame();

	// 호스트가 로비에서 방 설정을 수정했을 때 서버에 반영을 요청하는 RPC.
	// 값 검증(권한/범위)은 전적으로 서버가 하며, 클라이언트 입력은 신뢰하지 않는다.
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Lobby")
	void Server_UpdateRoomSettings(const FString& RoomTitle, const FString& Password,
		float BuildTime, int32 MaxPlacedShapeCount);
};
