#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SIPlayerController.generated.h"

// UI 위젯 클래스를 사용하기 위한 전방 선언
class UUserWidget;

/**
 * 플레이어의 입력을 처리하고 서버와 통신(RPC)하는 컨트롤러 클래스입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// ==========================================
	// [Client -> Server] 정답을 서버로 제출하는 RPC
	// ==========================================
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Game|Network")
	void Server_SubmitAnswer(const FString& Answer);

	// ==========================================
	// [Server -> Client] 출제자에게 정답(제시어)을 몰래 알려주는 RPC
	// ==========================================
	UFUNCTION(Client, Reliable)
	void Client_ReceiveSecretWord(const FString& SecretWord);


	// ==========================================
	// [개발자용 테스트 콘솔 명령어 (Exec)]
	// ==========================================

	UFUNCTION(Exec)
	void TestAnswer(const FString& Answer);

	UFUNCTION(Exec)
	void SetPhase(int32 PhaseIndex);

	UFUNCTION(Exec)
	void SetTime(int32 Seconds);


	// ==========================================
	// [Test -> Server RPC] 콘솔 명령을 서버에 적용하기 위한 함수
	// ==========================================

	UFUNCTION(Server, Reliable)
	void Server_TestSetPhase(int32 PhaseIndex);

	UFUNCTION(Server, Reliable)
	void Server_TestSetTime(int32 Seconds);


	// ==========================================
	// [UI 연동] 캐릭터(SICharacter) 측에서 호출할 함수
	// ==========================================

	/** 디테일 패널 위젯을 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* GetDetailPanelWidget() const;

protected:
	/** 실제 화면에 띄워질 디테일 패널 위젯 인스턴스 (블루프린트에서 생성 후 할당) */
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	UUserWidget* DetailPanelWidget;
};