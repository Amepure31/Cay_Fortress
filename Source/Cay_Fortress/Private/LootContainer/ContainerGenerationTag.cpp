// Fill out your copyright notice in the Description page of Project Settings.


#include "LootContainer/ContainerGenerationTag.h"
#include "LootContainer/LootContainerActor.h"

AContainerGenerationTag::AContainerGenerationTag()
{
	PrimaryActorTick.bCanEverTick = false;
	bEnableSpawn = true;
	ContainerType = EContainerType::FoodCrate;
}

TSubclassOf<ALootContainerActor> AContainerGenerationTag::ResolveContainerClass(TSubclassOf<ALootContainerActor> FallbackClass) const
{
	return ContainerClassOverride ? ContainerClassOverride : FallbackClass;
}

void AContainerGenerationTag::SetSpawnedContainer(ALootContainerActor* InContainer)
{
	SpawnedContainer = InContainer;
}
