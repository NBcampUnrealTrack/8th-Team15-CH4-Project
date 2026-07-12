// SILobbySettingWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SILobbySettingWidget.generated.h"

class ASIPlayerState;

/**
 * 로비 세팅 위젯. 방장 판별 및 게임 시작 요청을 담당합니다.
 * WBP 버튼 OnClicked → RequestStartGame() 을 호출하도록 바인딩하세요.
 */
UCLASS()
class TEAMPROJECT_API USILobbySettingWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	// 로컬 플레이어가 방장인지 (NativeConstruct 시점에 캐시됨)
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsLocalPlayerHost = false;

	// 게임 시작 버튼 클릭 시 BP에서 호출. 방장이면 서버에 요청, 아니면 무시.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestStartGame();

	// 방장 판별 결과가 확정된 후 BP에서 UI 상태(버튼 표시/숨김 등) 갱신용
	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnHostStatusResolved(bool bIsHost);

protected:
	virtual void NativeConstruct() override;

private:
	// 로컬 PlayerController의 SIPlayerState를 얻어 bIsHost 캐시
	void ResolveHostStatus();
};
