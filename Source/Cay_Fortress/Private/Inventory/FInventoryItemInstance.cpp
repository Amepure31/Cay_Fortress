// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryComponent.h"

void UInventoryItemInstance::EnsureContainerInventory(int32 InGridWidth, int32 InGridHeight)
{
	if (!ItemData || !ItemData->ItemData.ArmorStats.bIsContainer)
		return;

	if (!ContainerInventory)
	{
		ContainerInventory = NewObject<UInventoryComponent>(this);
		if (ContainerInventory)
		{
			ContainerInventory->bRejectContainerItems = true;
			ContainerInventory->InitializeGrid(FMath::Max(1, InGridWidth), FMath::Max(1, InGridHeight));
		}
	}
	else if (ContainerInventory->GridWidth != InGridWidth || ContainerInventory->GridHeight != InGridHeight)
	{
		ContainerInventory->InitializeGrid(FMath::Max(1, InGridWidth), FMath::Max(1, InGridHeight));
	}
}
