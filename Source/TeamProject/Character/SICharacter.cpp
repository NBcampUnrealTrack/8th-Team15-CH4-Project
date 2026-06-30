// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SICharacter.h"

// Sets default values
ASICharacter::ASICharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASICharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASICharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

