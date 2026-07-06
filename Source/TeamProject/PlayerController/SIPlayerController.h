#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SIPlayerController.generated.h"


class UDetailPanelWidget;
/**
 * �÷��̾��� �Է��� ó���ϰ� ������ ���(RPC)�ϴ� ��Ʈ�ѷ� Ŭ�����Դϴ�.
 */
UCLASS()
class TEAMPROJECT_API ASIPlayerController : public APlayerController
{
	GENERATED_BODY()
	
#pragma region GameMode

public:
	// ==========================================
	// [Client -> Server] ������ ������ �����ϴ� RPC
	// ==========================================
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Game|Network")
	void Server_SubmitAnswer(const FString& Answer);

	// ==========================================
	// [Server -> Client] �����ڿ��� ����(���þ�)�� ���� �˷��ִ� RPC
	// ==========================================
	UFUNCTION(Client, Reliable)
	void Client_ReceiveSecretWord(const FString& SecretWord);


	// ==========================================
	// [�����ڿ� �׽�Ʈ �ܼ� ��ɾ� (Exec)]
	// ==========================================

	UFUNCTION(Exec)
	void TestAnswer(const FString& Answer);

	UFUNCTION(Exec)
	void SetPhase(int32 PhaseIndex);

	UFUNCTION(Exec)
	void SetTime(int32 Seconds);


	// ==========================================
	// [Test -> Server RPC] �ܼ� ����� ������ �����ϱ� ���� �Լ�
	// ==========================================

	UFUNCTION(Server, Reliable)
	void Server_TestSetPhase(int32 PhaseIndex);

	UFUNCTION(Server, Reliable)
	void Server_TestSetTime(int32 Seconds);
	
#pragma endregion 

private:
	virtual void ReceivedPlayer() override;
	
#pragma region UI

protected:
	// 액터 변형 관련 UI Widget Class
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UDetailPanelWidget> DetailPanelWidget;

	// 액터 변형 관련 UI Widget Instance
	UPROPERTY()
	TObjectPtr<UDetailPanelWidget> DetailPanelWidgetInstance;

public:
	UDetailPanelWidget* GetDetailPanelWidget() const { return DetailPanelWidgetInstance; };

#pragma endregion
	

};