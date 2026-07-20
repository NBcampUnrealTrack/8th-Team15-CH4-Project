// SINameplateWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/SIUserWidget.h"
#include "SINameplateWidget.generated.h"

class UTextBlock;

/** 캐릭터 머리 위에 붙는 닉네임 표시. WidgetComponent가 3D 공간에 띄운다.
	이름의 원본은 서버가 복제하는 APlayerState::GetPlayerName()이며,
	갱신 시점은 ASICharacter가 관리한다(위젯은 받아서 그리기만 한다). */
UCLASS(Blueprintable)
class TEAMPROJECT_API USINameplateWidget : public USIUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void SetPlayerName(const FString& PlayerName);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerName;
};
