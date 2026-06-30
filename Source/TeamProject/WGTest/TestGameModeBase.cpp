// Fill out your copyright notice in the Description page of Project Settings.


#include "WGTest/TestGameModeBase.h"

#include "WGTest/TestCharacter.h"
#include "WGTest/TestPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ATestGameModeBase::ATestGameModeBase()
{
	DefaultPawnClass = ATestCharacter::StaticClass();
	PlayerControllerClass = ATestPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> TestCharacterClass(TEXT("/Game/Shape_It/Object/Test/Character/BP_TestCharacter"));
	if (TestCharacterClass.Succeeded())
	{
		DefaultPawnClass = TestCharacterClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> TestPlayerControllerClass(TEXT("/Game/Shape_It/Object/Test/GameController/BP_TestPlayerController"));
	if (TestPlayerControllerClass.Succeeded())
	{
		PlayerControllerClass = TestPlayerControllerClass.Class;
	}
}
