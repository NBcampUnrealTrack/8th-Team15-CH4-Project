// SIUIManagerComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SIUIManagerComponent.generated.h"

class UUserWidget;

UENUM()
enum class EUIType : uint8
{
	Exit,			// 종료 확인창
	CreateRoom,		// 방 만들기 설정
	RoomList,		// 방 목록
	LobbySetting,	// 방 설정 (로비 내)
	Participants	// 참가자 목록 (TAB)
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEAMPROJECT_API USIUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USIUIManagerComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditAnywhere, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TMap<EUIType, TSubclassOf<UUserWidget>> WidgetClasses;

	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> WidgetStack;

public:
	void OpenWidget(EUIType Type);

	void CloseWidget();

};
