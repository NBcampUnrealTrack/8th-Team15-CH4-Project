// SIPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Component/SIUIManagerComponent.h"
#include "Enums/SITypes.h"
#include "SIPlayerController.generated.h"

class ASIGameState;

class USIParticipantsListWidget;
class USIScoreBoardWidget;
class USIDrawingToolWidget;
class USIMainMenuWidget;
class USILobbySettingWidget;
class USIHUDWidget;

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

protected:
	virtual void BeginPlay() override;	
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;;
	
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
	
	// ==========================================
	// Chat
	// ==========================================
	UFUNCTION(Server, Reliable)
	void Server_SendChat(const FString& Message);
	
	
#pragma endregion 

#pragma region UI
	
	//========== UI Common Used Var & Func ==========
private:
	UPROPERTY(EditDefaultsOnly, Category= "IA_UICancel" , meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_ScoreBoardWidgetCancel;
	
	UPROPERTY(EditDefaultsOnly, Category= "IA_UICancel" , meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_ParticipantsListWidgetCancel;
	
	ESIGamePhase CurrentPhase = ESIGamePhase::None;
	
	TWeakObjectPtr<ASIGameState> GameState;
	
	FTimerHandle GameStateRetryHandle;
	
	FString CachedSecretWord;
	
private:
	virtual void ReceivedPlayer() override;

	virtual void SetupInputComponent() override;
	
	UFUNCTION()
	void HandlePhaseChanged(ESIGamePhase NewPhase);
	
	void TryCacheGameState();
	
	void RemovedScoreBoardWidget();
	
	void OpenParticipantsListWidget();
	
	void CloseParticipantsListWidget();
	
	void CloseAllPhaseWidgets();
	
	//========== LobbySettingWidget ==========
	UPROPERTY(EditDefaultsOnly, Category = "LobbySettingWidget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USILobbySettingWidget> LobbySettingWidgetClass;

	UPROPERTY()
	TObjectPtr<USILobbySettingWidget> LobbySettingWidget;

	UFUNCTION()
	void StartRoomCreation();

	void OpenLobbySettingWidget();

	void CloseLobbySettingWidget();
	
	//========== DrawingToolWidget ==========
	UPROPERTY(EditAnywhere, Category = "DrawingToolWidget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIDrawingToolWidget> DrawingToolWidgetClass;
	
public:
	UPROPERTY()
	TObjectPtr<USIDrawingToolWidget> DrawingToolWidget;
	
private:
	//========== HUDWidget ==========
	UPROPERTY(EditDefaultsOnly, Category= "HUDWidget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIHUDWidget> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<USIHUDWidget> HUDWidget;
	
	
	//========== ScoreBoardWidget ==========
	UPROPERTY(EditDefaultsOnly, Category= "ScoreBoardWidget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIScoreBoardWidget> ScoreBoardWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USIScoreBoardWidget> ScoreBoardWidget;
	
	//========== ParticipantsListWidget ==========
	UPROPERTY(EditDefaultsOnly, Category= "ParticipantsListWidget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIParticipantsListWidget> ParticipantsListWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USIParticipantsListWidget> ParticipantsListWidget;
	
#pragma endregion
	

};