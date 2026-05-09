#include "Equipment/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemType.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	Slots.Empty();
	Slots.Emplace(EEquipmentSlotType::Head);
	Slots.Emplace(EEquipmentSlotType::Chest);
	Slots.Emplace(EEquipmentSlotType::Backpack);
	Slots.Emplace(EEquipmentSlotType::Weapon1);
	Slots.Emplace(EEquipmentSlotType::Weapon2);
}

bool UEquipmentComponent::CanEquipItemToSlot(UInventoryItemDataAsset* ItemData, EEquipmentSlotType Slot) const
{
	if (!ItemData || Slot == EEquipmentSlotType::MAX)
	{
		return false;
	}

	const FInventoryItemData& Data = ItemData->ItemData;

	if (IsWeaponSlot(Slot))
	{
		return Data.ItemType == EInventoryItemType::Weapon;
	}

	if (IsArmorSlot(Slot))
	{
		if (Data.ItemType != EInventoryItemType::Armor)
		{
			return false;
		}

		const EArmorEquipSlot ArmorSlot = Data.ArmorStats.EquipSlot;

		if (ArmorSlot == EArmorEquipSlot::FullBody)
		{
			return true;
		}

		return ArmorEquipSlotToEquipmentSlot(static_cast<uint8>(ArmorSlot)) == Slot;
	}

	return false;
}

bool UEquipmentComponent::EquipItemFromInventory(UInventoryComponent* SourceInventory, UInventoryItemInstance* Item, EEquipmentSlotType Slot)
{
	if (!SourceInventory || !Item || !Item->ItemData)
	{
		return false;
	}

	if (!CanEquipItemToSlot(Item->ItemData, Slot))
	{
		return false;
	}

	FEquipmentSlotEntry* SlotEntry = FindSlotEntry(Slot);
	if (!SlotEntry)
	{
		return false;
	}

	if (SlotEntry->EquippedItem)
	{
		if (!UnequipItemToInventory(Slot, SourceInventory))
		{
			return false;
		}
	}

	SourceInventory->RemoveItem(Item);

	Item->SlotX = -1;
	Item->SlotY = -1;
	SlotEntry->EquippedItem = Item;

	OnEquipmentChanged.Broadcast(Slot, Item);
	return true;
}

bool UEquipmentComponent::UnequipItemToInventory(EEquipmentSlotType Slot, UInventoryComponent* TargetInventory)
{
	if (!TargetInventory)
	{
		return false;
	}

	FEquipmentSlotEntry* SlotEntry = FindSlotEntry(Slot);
	if (!SlotEntry || !SlotEntry->EquippedItem)
	{
		return false;
	}

	UInventoryItemInstance* Item = SlotEntry->EquippedItem;
	if (!Item->ItemData)
	{
		return false;
	}

	if (!TargetInventory->AddExistingItemInstance(Item))
	{
		return false;
	}

	SlotEntry->EquippedItem = nullptr;
	OnEquipmentChanged.Broadcast(Slot, nullptr);
	return true;
}

UInventoryItemInstance* UEquipmentComponent::GetEquippedItem(EEquipmentSlotType Slot) const
{
	const FEquipmentSlotEntry* SlotEntry = FindSlotEntry(Slot);
	return SlotEntry ? SlotEntry->EquippedItem : nullptr;
}

bool UEquipmentComponent::IsSlotOccupied(EEquipmentSlotType Slot) const
{
	return GetEquippedItem(Slot) != nullptr;
}

float UEquipmentComponent::GetTotalDamageReduction() const
{
	float Total = 0.0f;
	for (const FEquipmentSlotEntry& Entry : Slots)
	{
		if (!IsArmorSlot(Entry.SlotType) || !Entry.EquippedItem || !Entry.EquippedItem->ItemData)
		{
			continue;
		}
		Total += Entry.EquippedItem->ItemData->ItemData.ArmorStats.DamageReductionRatio;
	}
	return FMath::Clamp(Total, 0.0f, 1.0f);
}

float UEquipmentComponent::GetTotalArmorValue() const
{
	float Total = 0.0f;
	for (const FEquipmentSlotEntry& Entry : Slots)
	{
		if (!IsArmorSlot(Entry.SlotType) || !Entry.EquippedItem || !Entry.EquippedItem->ItemData)
		{
			continue;
		}
		Total += Entry.EquippedItem->ItemData->ItemData.ArmorStats.ArmorValue;
	}
	return Total;
}

FEquipmentSlotEntry* UEquipmentComponent::FindSlotEntry(EEquipmentSlotType Slot)
{
	for (FEquipmentSlotEntry& Entry : Slots)
	{
		if (Entry.SlotType == Slot)
		{
			return &Entry;
		}
	}
	return nullptr;
}

const FEquipmentSlotEntry* UEquipmentComponent::FindSlotEntry(EEquipmentSlotType Slot) const
{
	for (const FEquipmentSlotEntry& Entry : Slots)
	{
		if (Entry.SlotType == Slot)
		{
			return &Entry;
		}
	}
	return nullptr;
}

bool UEquipmentComponent::IsArmorSlot(EEquipmentSlotType Slot)
{
	return Slot == EEquipmentSlotType::Head
		|| Slot == EEquipmentSlotType::Chest
		|| Slot == EEquipmentSlotType::Backpack;
}

bool UEquipmentComponent::IsWeaponSlot(EEquipmentSlotType Slot)
{
	return Slot == EEquipmentSlotType::Weapon1 || Slot == EEquipmentSlotType::Weapon2;
}

EEquipmentSlotType UEquipmentComponent::ArmorEquipSlotToEquipmentSlot(uint8 ArmorSlot)
{
	switch (static_cast<EArmorEquipSlot>(ArmorSlot))
	{
	case EArmorEquipSlot::Head:     return EEquipmentSlotType::Head;
	case EArmorEquipSlot::Chest:    return EEquipmentSlotType::Chest;
	case EArmorEquipSlot::Backpack: return EEquipmentSlotType::Backpack;
	case EArmorEquipSlot::Arms:
	case EArmorEquipSlot::Legs:
		return EEquipmentSlotType::MAX;
	default:
		return EEquipmentSlotType::MAX;
	}
}
