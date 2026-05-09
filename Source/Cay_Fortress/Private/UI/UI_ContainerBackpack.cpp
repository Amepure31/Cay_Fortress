#include "UI/UI_ContainerBackpack.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"

void UUI_ContainerBackpack::NativeConstruct()
{
	Super::NativeConstruct();
}

void UUI_ContainerBackpack::NativeDestruct()
{
	ContainerItem = nullptr;
	ContainerInventory = nullptr;
	Super::NativeDestruct();
}

void UUI_ContainerBackpack::BindContainerBackpack(UInventoryItemInstance* InContainerItem)
{
	ContainerItem = InContainerItem;
	if (InContainerItem && InContainerItem->ItemData
		&& InContainerItem->ItemData->ItemData.ArmorStats.bIsContainer)
	{
		const FArmorItemStats& ArmorStats = InContainerItem->ItemData->ItemData.ArmorStats;
		InContainerItem->EnsureContainerInventory(ArmorStats.ContainerGridWidth, ArmorStats.ContainerGridHeight);
		ContainerInventory = InContainerItem->ContainerInventory;
		BindInventory(ContainerInventory.Get());
	}
}
