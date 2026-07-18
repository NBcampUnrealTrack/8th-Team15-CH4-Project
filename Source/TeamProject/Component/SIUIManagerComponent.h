// SIUIManagerComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameInstance/SISessionSubsystem.h"
#include "SIUIManagerComponent.generated.h"

class USIUserWidget;

UENUM()
enum class EUIType : uint8
{
	Exit,			// 종료 확인창
	CreateRoom,		// 방 만들기 설정
	RoomList,		// 방 목록
	LobbySetting,	// 방 설정 (로비 내)
	Notice,			// 확인 버튼 하나짜리 안내창
};

USTRUCT()
struct FUIStackEntry
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EUIType UIType = EUIType::Exit;

	UPROPERTY()
	TObjectPtr<USIUserWidget> Widget;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIConfirmed, EUIType, type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateRoomRequested, const FSICreateSessionParams&, payload);

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

public:
	UPROPERTY()
	FOnUIConfirmed OnUIConfirmed;

	UPROPERTY()
	FOnCreateRoomRequested OnCreateRoomRequested;

private:
	UFUNCTION()
	void OnWidgetConfirmed();

	UFUNCTION()
	void OnWidgetCancelled();

	UFUNCTION()
	void HandleCreateRoomRequested();

private:
	UPROPERTY(EditAnywhere, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TMap<EUIType, TSubclassOf<USIUserWidget>> WidgetClasses;

	UPROPERTY()
	TArray<FUIStackEntry> WidgetStack;

public:
	/** 연 위젯을 돌려준다 — 문구 주입처럼 생성 직후 손볼 게 있을 때 쓴다. 실패/중복이면 nullptr */
	USIUserWidget* OpenWidget(EUIType Type);

	void CloseWidget();
	
};
