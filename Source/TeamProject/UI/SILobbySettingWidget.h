// SILobbySettingWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SILobbySettingWidget.generated.h"

class ASIPlayerState;

class UButton;
class UEditableText;

UCLASS()
class TEAMPROJECT_API USILobbySettingWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsLocalPlayerHost = false;
	
private:
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UButton> Button_GameStart;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_RoomName;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_RoomPassword;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_BuildTimeLimit;
	
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_GuessTimeLimit;
	
	TWeakObjectPtr<ASIPlayerState> CachedPlayerState;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestStartGame();
	
private:	
	UFUNCTION()
	void HandleHostStatusChanged(bool bNewIsHost);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override; 
	
private:
	void ResolveHostStatus();

	FTimerHandle HostResolveRetryTimer;
	
};
