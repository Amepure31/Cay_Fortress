// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_LootContainer.h"
#include "Inventory/InventoryComponent.h"

void UUI_LootContainer::BindContainerInventory(UInventoryComponent* InContainerInventory)
{
	ContainerInventory = InContainerInventory;
	BindInventory(InContainerInventory);
}

