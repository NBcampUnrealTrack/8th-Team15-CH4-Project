// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "GameInstance/SISessionSubsystem.h"
#include "SITitlePlayerController.generated.h"

class UInputAction;
class USIUserWidget;
class USIUIManagerComponent;
class USIMainMenuWidget;
class USoundBase;
class UAudioComponent;
/**
 * 
 */
UCLASS()
class TEAMPROJECT_API ASITitlePlayerController : public APlayerController
{
	GENERATED_BODY()
	
#pragma region APlayerController override
	
public:
	ASITitlePlayerController();
	
private:
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USIMainMenuWidget> MainMenuWidgetClass;
	
	UPROPERTY(EditAnywhere, Category = "UIManagerComponent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USIUIManagerComponent> UIManagerComponent;
	
private:
	virtual void ReceivedPlayer() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void SetupInputComponent() override;

#pragma endregion
	
#pragma region Input
	
private:
	//========== Remove UI ==========
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_UICancel;
	
private:
	void HandleCancel();
	
#pragma endregion
	
#pragma region Function
	
private:
	UFUNCTION()
	void HandleCreateRoom();

	UFUNCTION()
	void HandleJoinRoom();

	UFUNCTION()
	void HandleQuit();
	
	UFUNCTION()
	void OnCreateRoomClicked(const FSICreateSessionParams& Settings);

	/** 방 생성 결과 — 실패했을 때만 안내한다 (성공 시엔 GameInstance가 로비를 연다) */
	UFUNCTION()
	void HandleCreateSessionResult(bool bWasSuccessful);

	/** 직전에 방에서 튕겨 나왔다면 그 사유를 안내창으로 띄운다 */
	void ShowPendingFailureNotice();

	/** 확인 버튼 하나짜리 안내창을 띄운다 */
	void ShowNotice(const FText& Message);

	static FText MakeFailureMessage(ESIConnectionFailureType FailureType);
	
#pragma endregion

#pragma region Sound

protected:
	UPROPERTY(VisibleAnywhere,Category = "Audio")
	TObjectPtr<UAudioComponent> BGMAudioComponent;
     
	UPROPERTY(EditAnywhere,Category = "Audio")
	TObjectPtr<USoundBase> MainMenuBGM;
     
	void InitializeMainMenuBGM();
    
#pragma endregion
};
