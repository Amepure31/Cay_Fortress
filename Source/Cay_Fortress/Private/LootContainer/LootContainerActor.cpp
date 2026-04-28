// Fill out your copyright notice in the Description page of Project Settings.

#include "LootContainer/LootContainerActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/InventoryItemRarity.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	constexpr float FoodRaritySigma = 1.35f;
	static const FName ItemDataFolderPath(TEXT("/Game/Inventory/InventoryItemDataAsset"));

static TArray<UInventoryItemDataAsset*> GetAllItemDataAssetsForContainerRefresh()
{
	TArray<UInventoryItemDataAsset*> Result;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(ItemDataFolderPath, AssetDataList, true);

	TSet<const UInventoryItemDataAsset*> UniqueAssets;
	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* RawAsset = AssetData.GetAsset();
		UInventoryItemDataAsset* Asset = Cast<UInventoryItemDataAsset>(RawAsset);
		if (!Asset)
		{
			if (const UBlueprint* BlueprintAsset = Cast<UBlueprint>(RawAsset))
			{
				if (BlueprintAsset->GeneratedClass && BlueprintAsset->GeneratedClass->IsChildOf(UInventoryItemDataAsset::StaticClass()))
				{
					Asset = Cast<UInventoryItemDataAsset>(BlueprintAsset->GeneratedClass->GetDefaultObject());
				}
			}
		}

		if (Asset && !UniqueAssets.Contains(Asset))
		{
			UniqueAssets.Add(Asset);
			Result.Add(Asset);
		}
	}

	return Result;
}

static EInventoryItemRarity RollFoodRarityByNormalDistribution()
{
	TArray<float> Weights;
	Weights.SetNum(static_cast<int32>(EInventoryItemRarity::MAX));

	float TotalWeight = 0.0f;
	for (int32 RarityIndex = 0; RarityIndex < Weights.Num(); ++RarityIndex)
	{
		const float X = static_cast<float>(RarityIndex);
		const float Weight = FMath::Exp(-0.5f * FMath::Square(X / FoodRaritySigma));
		Weights[RarityIndex] = Weight;
		TotalWeight += Weight;
	}

	if (TotalWeight <= KINDA_SMALL_NUMBER)
	{
		return EInventoryItemRarity::Normal;
	}

	const float Roll = FMath::FRandRange(0.0f, TotalWeight);
	float Accumulated = 0.0f;
	for (int32 RarityIndex = 0; RarityIndex < Weights.Num(); ++RarityIndex)
	{
		Accumulated += Weights[RarityIndex];
		if (Roll <= Accumulated)
		{
			return static_cast<EInventoryItemRarity>(RarityIndex);
		}
	}

	return EInventoryItemRarity::Normal;
}

static int32 RollStackSizeByThirdRule(const FInventoryItemData& ItemData)
{
	if (!ItemData.bCanStack || ItemData.MaxStackSize <= 1)
	{
		return 1;
	}

	const int32 MinStack = FMath::Max(1, ItemData.MaxStackSize / 3);
	const int32 ExtraRange = FMath::Max(0, ItemData.MaxStackSize - MinStack);
	return MinStack + FMath::RandRange(0, ExtraRange);
}

static UInventoryItemDataAsset* PickAssetByRarity(
	const TArray<UInventoryItemDataAsset*>& CandidateAssets,
	EInventoryItemRarity TargetRarity)
{
	TArray<UInventoryItemDataAsset*> RarityCandidates;
	for (UInventoryItemDataAsset* Asset : CandidateAssets)
	{
		if (Asset && Asset->ItemData.Rarity == TargetRarity)
		{
			RarityCandidates.Add(Asset);
		}
	}

	const TArray<UInventoryItemDataAsset*>& Pool = RarityCandidates.IsEmpty() ? CandidateAssets : RarityCandidates;
	if (Pool.IsEmpty())
	{
		return nullptr;
	}

	return Pool[FMath::RandRange(0, Pool.Num() - 1)];
}

static UInventoryItemDataAsset* LoadInventoryItemDataAssetFromBlueprintPaths(
	const TCHAR* PrimaryPath,
	const TCHAR* FallbackPath,
	const FName TargetPackageName)
{
	if (UInventoryItemDataAsset* Asset = LoadObject<UInventoryItemDataAsset>(nullptr, PrimaryPath))
	{
		return Asset;
	}
	if (UInventoryItemDataAsset* Asset = LoadObject<UInventoryItemDataAsset>(nullptr, FallbackPath))
	{
		return Asset;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPackageName(TargetPackageName, AssetDataList);
	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* RawAsset = AssetData.GetAsset();
		if (UInventoryItemDataAsset* Asset = Cast<UInventoryItemDataAsset>(RawAsset))
		{
			return Asset;
		}

		if (const UBlueprint* BlueprintAsset = Cast<UBlueprint>(RawAsset))
		{
			if (BlueprintAsset->GeneratedClass && BlueprintAsset->GeneratedClass->IsChildOf(UInventoryItemDataAsset::StaticClass()))
			{
				if (UInventoryItemDataAsset* AssetCDO = Cast<UInventoryItemDataAsset>(BlueprintAsset->GeneratedClass->GetDefaultObject()))
				{
					return AssetCDO;
				}
			}
		}
	}

	return nullptr;
}

static UInventoryItemDataAsset* GetToiletFreshWaterAsset()
{
	return LoadInventoryItemDataAssetFromBlueprintPaths(
		TEXT("/Game/Inventory/InventoryItemDataAsset/Food/BP_FreshWater_DA.BP_FreshWater_DA"),
		TEXT("/Game/Inventory/InventoryItemDataAsset/Food/BP_FreshWater_DA"),
		FName(TEXT("/Game/Inventory/InventoryItemDataAsset/Food/BP_FreshWater_DA")));
}

static UInventoryItemDataAsset* GetEquipmentHelmetAsset()
{
	return LoadInventoryItemDataAssetFromBlueprintPaths(
		TEXT("/Game/Inventory/InventoryItemDataAsset/Weapon/BP_Helmet_DA.BP_Helmet_DA"),
		TEXT("/Game/Inventory/InventoryItemDataAsset/Weapon/BP_Helmet_DA"),
		FName(TEXT("/Game/Inventory/InventoryItemDataAsset/Weapon/BP_Helmet_DA")));
}

static UInventoryItemDataAsset* GetEquipmentArmorAsset()
{
	return LoadInventoryItemDataAssetFromBlueprintPaths(
		TEXT("/Game/Inventory/InventoryItemDataAsset/Weapon/BP_Armor_DA.BP_Armor_DA"),
		TEXT("/Game/Inventory/InventoryItemDataAsset/Weapon/BP_Armor_DA"),
		FName(TEXT("/Game/Inventory/InventoryItemDataAsset/Weapon/BP_Armor_DA")));
}
}

ALootContainerActor::ALootContainerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	ContainerMesh->SetupAttachment(Root);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	ContainerGridWidth = 6;
	ContainerGridHeight = 5;
	ContainerType = EContainerType::FoodCrate;
}

void ALootContainerActor::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryComponent)
	{
		InventoryComponent->InitializeGrid(
			FMath::Max(1, ContainerGridWidth),
			FMath::Max(1, ContainerGridHeight));
		InventoryComponent->CloseInventory();
	}
}

void ALootContainerActor::OpenContainer()
{
	if (InventoryComponent)
	{
		InventoryComponent->OpenInventory();
	}
}

void ALootContainerActor::CloseContainer()
{
	if (InventoryComponent)
	{
		InventoryComponent->CloseInventory();
	}
}

bool ALootContainerActor::IsContainerOpen() const
{
	return InventoryComponent && InventoryComponent->IsInventoryOpen();
}

void ALootContainerActor::RefreshItemsByRoom_Implementation()
{
	if (!InventoryComponent)
	{
		return;
	}

	if (ContainerType == EContainerType::EquipmentCrate)
	{
		InventoryComponent->Clear();
		UInventoryItemDataAsset* HelmetAsset = GetEquipmentHelmetAsset();
		UInventoryItemDataAsset* ArmorAsset = GetEquipmentArmorAsset();
		UInventoryItemDataAsset* ChosenAsset = nullptr;
		if (HelmetAsset && ArmorAsset)
		{
			ChosenAsset = FMath::RandBool() ? HelmetAsset : ArmorAsset;
		}
		else
		{
			ChosenAsset = HelmetAsset ? HelmetAsset : ArmorAsset;
		}
		if (ChosenAsset)
		{
			InventoryComponent->AddItem(ChosenAsset, RollStackSizeByThirdRule(ChosenAsset->ItemData));
		}
		return;
	}

	// Only handle requested container types for now.
	if (ContainerType != EContainerType::FoodCrate && ContainerType != EContainerType::Toilet)
	{
		return;
	}

	TArray<UInventoryItemDataAsset*> CandidateAssets;
	if (ContainerType == EContainerType::Toilet)
	{
		if (UInventoryItemDataAsset* FreshWaterAsset = GetToiletFreshWaterAsset())
		{
			CandidateAssets.Add(FreshWaterAsset);
		}
	}
	else
	{
		const TArray<UInventoryItemDataAsset*> AllItemAssets = GetAllItemDataAssetsForContainerRefresh();
		if (AllItemAssets.IsEmpty())
		{
			InventoryComponent->Clear();
			return;
		}

		for (UInventoryItemDataAsset* ItemDataAsset : AllItemAssets)
		{
			if (!ItemDataAsset)
			{
				continue;
			}

			const FInventoryItemData& ItemData = ItemDataAsset->ItemData;
			if (ItemData.ItemType == EInventoryItemType::Food)
			{
				CandidateAssets.Add(ItemDataAsset);
			}
		}
	}

	InventoryComponent->Clear();
	if (CandidateAssets.IsEmpty())
	{
		return;
	}

	// Toilet refresh rule:
	// - 25% empty
	// - 75% spawn fresh water once
	// - stack uses 1/3 guaranteed + 2/3 random rule
	if (ContainerType == EContainerType::Toilet)
	{
		const bool bEmptyRefresh = FMath::FRand() < 0.25f;
		if (bEmptyRefresh)
		{
			return;
		}

		UInventoryItemDataAsset* FreshWaterAsset = CandidateAssets[0];
		if (FreshWaterAsset)
		{
			InventoryComponent->AddItem(FreshWaterAsset, RollStackSizeByThirdRule(FreshWaterAsset->ItemData));
		}
		return;
	}

	const int32 SpawnCount = FMath::RandRange(0, 3);
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const EInventoryItemRarity TargetRarity = RollFoodRarityByNormalDistribution();
		UInventoryItemDataAsset* ChosenAsset = PickAssetByRarity(CandidateAssets, TargetRarity);
		if (!ChosenAsset)
		{
			continue;
		}

		InventoryComponent->AddItem(ChosenAsset, RollStackSizeByThirdRule(ChosenAsset->ItemData));
	}
}

bool ALootContainerActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && InventoryComponent != nullptr;
}

void ALootContainerActor::Interact_Implementation(AActor* Interactor)
{
	if (CanInteract_Implementation(Interactor))
	{
		OpenContainer();
	}
}

FText ALootContainerActor::GetInteractText_Implementation() const
{
	return FText::FromString(TEXT("按E打开容器"));
}

