// SIPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Component/SIUIManagerComponent.h"
#include "SIPlayerController.generated.h"

class USIUIManagerComponent;

class USIDrawingToolWidget;
class USIMainMenuWidget;
class USILobbySettingWidget;

class UInputAction;


/**
 * 플레이어의 입력을 처리하고 서버와 통신(RPC)하는 컨트롤러 클래스입니다.
 */
UCLASS()
class TEAMPROJECT_API ASIPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ASIPlayerController();


#pragma region GameMode

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

#pragma endregion 

#pragma region UI

	//========== DrawingTool UI Test ==========
protected:
	// 액터 변형 관련 UI Widget Class
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<USIDrawingToolWidget> DrawingToolWidget;

	// 액터 변형 관련 UI Widget Instance
	UPROPERTY()
	TObjectPtr<USIDrawingToolWidget> DrawingToolWidgetInstance;

public:
	USIDrawingToolWidget* GetDrawingToolWidget() const { return DrawingToolWidgetInstance; };

	//========== MainMenuWidget ==========
private:
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIMainMenuWidget> MainMenuWidgetClass;;

private:
	UFUNCTION()
	void HandleCreateRoom();

	UFUNCTION()
	void HandleJoinRoom();

	UFUNCTION()
	void HandleQuit();

	//========== LobbySettingWidget ==========
private:
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USILobbySettingWidget> LobbySettingWidgetClass;

	UPROPERTY()
	TObjectPtr<USILobbySettingWidget> LobbySettingWidget;

private:
	UFUNCTION()
	void HandleCreateRoomConfirmed();

	void OpenLobbySettingWidget();

	void CloseLobbySettingWidget();

	//========== Remove UI ==========
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_UICancel;


	//========== UI Comp & FUNCS ==========
private:
	UPROPERTY(EditAnywhere, Category = "UIManagerComponent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USIUIManagerComponent> UIManagerComponent;


private:
	virtual void ReceivedPlayer() override;

	virtual void SetupInputComponent() override;

	void HandleUIConfirmed(EUIType type);

	void HandleCancel();
#pragma endregion
	

};