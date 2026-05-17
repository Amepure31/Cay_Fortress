#pragma once

#include "CoreMinimal.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Alex_PlayerCharacter.h"
#include "BaseBuilding/StorageCabinetActor.h"
#include "EngineUtils.h"

template<typename T>
static T* TryLoadDataAsset(const FSoftObjectPath& Path)
{
	if (!Path.IsValid()) return nullptr;
	UObject* Loaded = Path.TryLoad();
	if (!Loaded) return nullptr;
	if (T* Direct = Cast<T>(Loaded)) return Direct;
	if (UBlueprint* BP = Cast<UBlueprint>(Loaded))
		if (BP->GeneratedClass && BP->GeneratedClass->IsChildOf(T::StaticClass()))
			return Cast<T>(BP->GeneratedClass->GetDefaultObject());
	return nullptr;
}

static void CollectAllSources(AActor* Interactor, TArray<UInventoryComponent*>& OutSources)
{
	if (AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(Interactor))
	{
		if (UInventoryComponent* PlayerInv = Player->FindComponentByClass<UInventoryComponent>())
			OutSources.Add(PlayerInv);
	}
	if (UWorld* World = Interactor ? Interactor->GetWorld() : nullptr)
	{
		for (TActorIterator<AStorageCabinetActor> It(World); It; ++It)
		{
			if (UInventoryComponent* CabInv = (*It)->GetInventoryComponent())
				OutSources.Add(CabInv);
		}
	}
}

static void CollectAllSources(AActor* Interactor, UInventoryComponent* SelfInv, TArray<UInventoryComponent*>& OutSources)
{
	if (AAlex_PlayerCharacter* Player = Cast<AAlex_PlayerCharacter>(Interactor))
		if (UInventoryComponent* PlayerInv = Player->FindComponentByClass<UInventoryComponent>())
			OutSources.Add(PlayerInv);
	if (SelfInv)
		OutSources.Add(SelfInv);
	if (UWorld* World = Interactor ? Interactor->GetWorld() : nullptr)
		for (TActorIterator<AStorageCabinetActor> It(World); It; ++It)
			if (UInventoryComponent* CabInv = (*It)->GetInventoryComponent())
				if (CabInv != SelfInv)
					OutSources.Add(CabInv);
}

static int32 ConsumeFromSources(UInventoryItemDataAsset* CostItem, int32 Amount, const TArray<UInventoryComponent*>& Sources)
{
	int32 Remaining = Amount;
	for (UInventoryComponent* Source : Sources)
	{
		if (!Source || Remaining <= 0) continue;
		const int32 Before = Remaining;
		Remaining -= Source->ConsumeItemByDataAsset(CostItem, Remaining);
	}
	return Amount - Remaining;
}
