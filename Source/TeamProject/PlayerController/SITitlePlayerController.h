// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
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
	void OnCreateRoomClicked();
	
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
