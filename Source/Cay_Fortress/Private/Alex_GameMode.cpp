// Fill out your copyright notice in the Description page of Project Settings.


#include "Alex_GameMode.h"
#include "Alex_PlayerCharacter.h"
#include "Alex_PlayerController.h"
#include "UObject/SoftObjectPtr.h"

AAlex_GameMode::AAlex_GameMode()
{
	static ConstructorHelpers::FClassFinder<AAlex_PlayerCharacter> PlayerPawnBPClass(TEXT("/Game/Characters/Model/BP_Alex_PlayerCharacter.BP_Alex_PlayerCharacter_C"));
	if (PlayerPawnBPClass.Succeeded())
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	
	static ConstructorHelpers::FClassFinder<AAlex_PlayerController> PlayerControllerBPClass(TEXT("/Game/Input/BP_Alex_PlayerController.BP_Alex_PlayerController_C"));
	if (PlayerControllerBPClass.Succeeded())
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}
	
}

void AAlex_GameMode::BeginPlay()
{
	Super::BeginPlay();
}
