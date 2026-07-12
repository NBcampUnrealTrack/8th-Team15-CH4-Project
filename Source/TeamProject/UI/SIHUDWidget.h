// SIHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "Enums/SITypes.h"
#include "SIHUDWidget.generated.h"

class ASIGameState;
class APlayerState;

/**
 * 인게임 HUD. GameState의 페이즈/타이머/발표자/채팅 이벤트를 받아 상태를 캐시합니다.
 * 실제 텍스트/이미지는 BP에서 캐시 값을 참조해 표시하세요.
 */
UCLASS()
class TEAMPROJECT_API USIHUDWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	// 서버에서 넘겨준 이번 턴 제시어 (본인이 발표자일 때만 채워짐)
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	FString CurrentSecretWord;

	// 현재 게임 페이즈 캐시
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	ESIGamePhase CurrentPhase = ESIGamePhase::None;

	// 남은 시간(초) 캐시
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 RemainingTime = 0;

	// 현재 발표자 캐시
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<APlayerState> CurrentPresenter = nullptr;

	// PlayerController가 Client_ReceiveSecretWord에서 이 함수를 호출해 제시어 세팅
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetSecretWord(const FString& NewSecretWord);

	// BP에서 페이즈 텍스트 갱신 등 커스텀 반응이 필요하면 오버라이드해 사용
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHUDPhaseChanged(ESIGamePhase NewPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHUDTimeUpdated(int32 NewTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHUDPresenterChanged(APlayerState* NewPresenter);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHUDChatMessage(const FChatMessagePayload& Payload);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHUDSecretWordReceived(const FString& SecretWord);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePhaseChanged(ESIGamePhase NewPhase);

	UFUNCTION()
	void HandleTimeUpdated(int32 NewTime);

	UFUNCTION()
	void HandlePresenterChanged(APlayerState* NewPresenter);

	UFUNCTION()
	void HandleChatMessage(const FChatMessagePayload& Payload);

	// 바인딩된 GameState (해제 시 참조 필요)
	TWeakObjectPtr<ASIGameState> CachedGameState;
};
