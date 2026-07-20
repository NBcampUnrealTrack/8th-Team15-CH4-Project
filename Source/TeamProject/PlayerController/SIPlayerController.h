// SIPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Component/SIUIManagerComponent.h"
#include "Enums/SITypes.h"
#include "Components/AudioComponent.h"
#include "SIPlayerController.generated.h"

class ASIGameState;

class USIParticipantsListWidget;
class USIControlGuideWidget;
class USIScoreBoardWidget;
class USIDrawingToolWidget;
class USIMainMenuWidget;
class USILobbySettingWidget;
class USIHUDWidget;
class USIUserWidget;
class UAudioComponent;
class UInputAction;
class USoundBase;


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

public:
	// Enter 입력 시 채팅창에 포커스를 주고 GameAndUI 입력 모드로 전환한다.
	void FocusChat();

	// 채팅 전송/취소 후 다시 GameOnly 입력 모드로 복귀한다. (HUD가 커밋 시 호출)
	void EndChatFocus();

	/** 지금 화면에 있는 채팅창을 알려준다. 인게임 HUD와 로비 방 설정 위젯이
		각자 생성/소멸 시점에 자기를 등록/해제하고, Enter 포커스는 여기 등록된 쪽으로 간다.
		HUD를 직접 가리키면 HUD가 없는 로비에서 Enter가 먹지 않는다. */
	void RegisterChatWidget(USIUserWidget* ChatWidget);
	void UnregisterChatWidget(const USIUserWidget* ChatWidget);

private:
	// 현재 채팅 입력 모드인지 (Enter 중복 처리 방지)
	bool bChatFocused = false;

	// 현재 활성 채팅창 (인게임 HUD 또는 로비 위젯). 레벨과 함께 사라지므로 약참조.
	TWeakObjectPtr<USIUserWidget> ActiveChatWidget;

	/** 채팅 로그의 저장 주체. 위젯이 아니라 PlayerController가 맡는다 —
		위젯은 페이즈마다 생성·파괴되지만 PC는 레벨 내내 살아 있어
		HUD가 없는 로비/결과 화면의 채팅도 빠짐없이 기록된다. 위젯은 읽어서 그리기만 한다. */
	UFUNCTION()
	void HandleChatMessageForHistory(const FChatMessagePayload& Payload);

	void BindChatHistoryRecorder();

	FTimerHandle ChatRecorderRetryTimer;
	TWeakObjectPtr<ASIGameState> ChatRecorderBoundGameState;

private:
	
	UFUNCTION()
	void HandlePhaseChanged(ESIGamePhase NewPhase);
	
	void TryCacheGameState();
	
	void RemovedScoreBoardWidget();
	
	void OpenParticipantsListWidget();

	void CloseParticipantsListWidget();

	// F1을 누르고 있는 동안만 조작 가이드를 띄운다 (제작/정답 단계 한정)
	void OpenControlGuideWidget();

	void CloseControlGuideWidget();

	void CloseAllPhaseWidgets();

	UFUNCTION()
	void HandleDrawingToolColorSelected(int32 ColorIndex, FLinearColor Color);
	
	//========== LobbySettingWidget ==========
	UPROPERTY(EditDefaultsOnly, Category = "LobbySettingWidget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USILobbySettingWidget> LobbySettingWidgetClass;

	UPROPERTY()
	TObjectPtr<USILobbySettingWidget> LobbySettingWidget;

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

	//========== ControlGuideWidget ==========
	UPROPERTY(EditDefaultsOnly, Category= "ControlGuideWidget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIControlGuideWidget> ControlGuideWidgetClass;

	UPROPERTY()
	TObjectPtr<USIControlGuideWidget> ControlGuideWidget;

	#pragma endregion
	
#pragma region Sound
	UPROPERTY(EditAnywhere, Category = "UI|Sound")
	TObjectPtr<USoundBase> TabOpenSound;
	
	UPROPERTY(EditAnywhere, Category = "UI|Sound")
	TObjectPtr<USoundBase> TabCloseSound;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> ResultSFX;
	
	UPROPERTY(VisibleAnywhere, Category = "Sound")
	TObjectPtr<UAudioComponent> BGMAudioComponent;
	
	

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> BGMMetaSound;
	
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> LobbyBGM;
	
	void UpdateBGMParameters(ESIGamePhase NewPhase);
	
	void InitializeLevelBGM();
	
#pragma endregion
};
