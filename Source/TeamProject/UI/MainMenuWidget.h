// MainMenuWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "MainMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClickedCreateRoomButton);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClickedJoinRoomButton);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClickedQuitButton);

class UButton;

UCLASS()
class TEAMPROJECT_API UMainMenuWidget : public USIUserWidget
{
	GENERATED_BODY()
	
private:
	virtual void NativeConstruct() override;

public:
	UPROPERTY()
	FOnClickedCreateRoomButton OnClickedCreateRoomButton;

	UPROPERTY()
	FOnClickedJoinRoomButton OnClickedJoinRoomButton;

	UPROPERTY()
	FOnClickedQuitButton OnClickedQuitButton;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_CreateRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_JoinRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Quit;

private:
	UFUNCTION()
	void HandleCreateRoomClicked();

	UFUNCTION()
	void HandleJoinRoomClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
