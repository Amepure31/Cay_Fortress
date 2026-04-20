// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_LootContainer.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Math/UnrealMathUtility.h"

namespace
{
	const FString RefreshFoodOption = TEXT("刷新食物");
	constexpr float FoodRaritySigma = 1.35f;
}

void UUI_LootContainer::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &UUI_LootContainer::OnRefreshButtonClicked);
	}

	if (RefreshTypeComboBox)
	{
		RefreshTypeComboBox->OnSelectionChanged.AddDynamic(this, &UUI_LootContainer::OnRefreshTypeSelectionChanged);
		EnsureRefreshOptions();
		SetRefreshTypeListVisible(false);
	}
}

void UUI_LootContainer::NativeDestruct()
{
	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &UUI_LootContainer::OnRefreshButtonClicked);
	}

	if (RefreshTypeComboBox)
	{
		RefreshTypeComboBox->OnSelectionChanged.RemoveDynamic(this, &UUI_LootContainer::OnRefreshTypeSelectionChanged);
	}

	Super::NativeDestruct();
}

void UUI_LootContainer::BindContainerInventory(UInventoryComponent* InContainerInventory)
{
	ContainerInventory = InContainerInventory;
	BindInventory(InContainerInventory);
}

void UUI_LootContainer::OnRefreshButtonClicked()
{
	if (!RefreshTypeComboBox)
	{
		return;
	}

	EnsureRefreshOptions();
	const bool bVisible = RefreshTypeComboBox->GetVisibility() == ESlateVisibility::Visible;
	SetRefreshTypeListVisible(!bVisible);
}

void UUI_LootContainer::OnRefreshTypeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}

	if (SelectedItem == RefreshFoodOption)
	{
		RefreshFoodItems();
	}

	if (RefreshTypeComboBox)
	{
		RefreshTypeComboBox->ClearSelection();
	}
	SetRefreshTypeListVisible(false);
}

void UUI_LootContainer::RefreshFoodItems()
{
	if (!ContainerInventory)
	{
		return;
	}

	TArray<UInventoryItemDataAsset*> FoodItemAssets;
	const TArray<UInventoryItemDataAsset*> AllItemAssets = GetAllItemDataAssets();
	for (UInventoryItemDataAsset* ItemDataAsset : AllItemAssets)
	{
		if (!ItemDataAsset)
		{
			continue;
		}

		const FInventoryItemData& ItemData = ItemDataAsset->ItemData;
		if (ItemData.ItemType == EInventoryItemType::Food)
		{
			FoodItemAssets.Add(ItemDataAsset);
		}
	}

	ContainerInventory->Clear();

	if (FoodItemAssets.IsEmpty())
	{
		UpdateInventory();
		return;
	}

	const int32 SpawnCount = FMath::RandRange(0, 3);
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const EInventoryItemRarity TargetRarity = RollFoodRarityByNormalDistribution();

		TArray<UInventoryItemDataAsset*> RarityCandidates;
		for (UInventoryItemDataAsset* FoodAsset : FoodItemAssets)
		{
			if (!FoodAsset)
			{
				continue;
			}

			if (FoodAsset->ItemData.Rarity == TargetRarity)
			{
				RarityCandidates.Add(FoodAsset);
			}
		}

		const TArray<UInventoryItemDataAsset*>& CandidatePool = RarityCandidates.IsEmpty() ? FoodItemAssets : RarityCandidates;
		if (CandidatePool.IsEmpty())
		{
			continue;
		}

		UInventoryItemDataAsset* ChosenFoodAsset = CandidatePool[FMath::RandRange(0, CandidatePool.Num() - 1)];
		if (!ChosenFoodAsset)
		{
			continue;
		}

		const FInventoryItemData& ChosenData = ChosenFoodAsset->ItemData;
		int32 StackSizeToSpawn = 1;
		if (ChosenData.bCanStack && ChosenData.MaxStackSize > 1)
		{
			const int32 MinStack = FMath::Max(1, ChosenData.MaxStackSize / 3);
			const int32 ExtraRange = FMath::Max(0, ChosenData.MaxStackSize - MinStack);
			StackSizeToSpawn = MinStack + FMath::RandRange(0, ExtraRange);
		}

		ContainerInventory->AddItem(ChosenFoodAsset, StackSizeToSpawn);
	}

	UpdateInventory();
}

void UUI_LootContainer::EnsureRefreshOptions()
{
	if (!RefreshTypeComboBox)
	{
		return;
	}

	if (RefreshTypeComboBox->FindOptionIndex(RefreshFoodOption) == INDEX_NONE)
	{
		RefreshTypeComboBox->AddOption(RefreshFoodOption);
	}
}

void UUI_LootContainer::SetRefreshTypeListVisible(bool bVisible)
{
	if (!RefreshTypeComboBox)
	{
		return;
	}

	RefreshTypeComboBox->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

EInventoryItemRarity UUI_LootContainer::RollFoodRarityByNormalDistribution() const
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

