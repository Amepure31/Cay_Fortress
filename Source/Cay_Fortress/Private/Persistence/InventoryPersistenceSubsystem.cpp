#include "Persistence/InventoryPersistenceSubsystem.h"
#include "Alex_PlayerCharacter.h"
#include "Alex_PlayerController.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Equipment/EquipmentComponent.h"
#include "Engine/Blueprint.h"

DEFINE_LOG_CATEGORY_STATIC(LogInventoryPersistence, Log, All);

// ---- Capture ---------------------------------------------------------------

void UInventoryPersistenceSubsystem::CaptureFromPlayer(AAlex_PlayerCharacter* Player)
{
	SavedInventoryItems.Empty();
	SavedEquipmentSlots.Empty();
	bHasData = false;

	if (!Player)
	{
		//UE_LOG(LogInventoryPersistence, Warning, TEXT("[Persist] CaptureFromPlayer — Player is null"));
		return;
	}

	UInventoryComponent* Inv = Player->GetInventory();
	UEquipmentComponent* Equip = Player->GetEquipment();

	if (!Inv)
	{
		//UE_LOG(LogInventoryPersistence, Warning, TEXT("[Persist] CaptureFromPlayer — Inventory component is null"));
		return;
	}

	SavedGridWidth = Inv->GridWidth;
	SavedGridHeight = Inv->GridHeight;

	// Save player stats
	SavedPlayerStats.Health = Player->GetHealth();
	SavedPlayerStats.MaxHealth = Player->GetMaxHealth();
	SavedPlayerStats.Stamina = Player->GetStamina();
	SavedPlayerStats.MaxStamina = Player->GetMaxStamina();
	SavedPlayerStats.StaminaRecoveryRate = Player->StaminaRecoveryRate;
	SavedPlayerStats.Hunger = Player->GetHunger();
	SavedPlayerStats.MaxHunger = Player->GetMaxHunger();
	SavedPlayerStats.HungerDrainRate = Player->HungerDrainRate;
	SavedPlayerStats.Hydration = Player->GetHydration();
	SavedPlayerStats.MaxHydration = Player->GetMaxHydration();
	SavedPlayerStats.HydrationDrainRate = Player->HydrationDrainRate;

	//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] Captured — Inventory %dx%d, %d items, Stats HP=%.0f/%.0f"),
		//SavedGridWidth, SavedGridHeight, Inv->Items.Num(),
		//SavedPlayerStats.Health, SavedPlayerStats.MaxHealth);

	// Save inventory items
	SavedContainerItems.Empty();
	for (int32 i = 0; i < Inv->Items.Num(); ++i)
	{
		UInventoryItemInstance* Item = Inv->Items[i];
		if (!Item) continue;

		const int32 ItemIndex = SavedInventoryItems.Num();
		FPersistedItemData Data;
		CaptureItem(Item, Data);
		SavedInventoryItems.Add(Data);

		// Capture nested container-backpack items
		if (Item->ContainerInventory)
		{
			TArray<FPersistedItemData> Children;
			for (const TObjectPtr<UInventoryItemInstance>& CItem : Item->ContainerInventory->Items)
			{
				if (!CItem) continue;
				FPersistedItemData CData;
				CaptureItem(CItem, CData);
				Children.Add(CData);
			}
			if (Children.Num() > 0)
			{
				FPersistedItemDataArray& Arr = SavedContainerItems.Add(ItemIndex);
				Arr.Items = MoveTemp(Children);
			}
		}
	}

	// Save equipment slots
	if (Equip)
	{
		const TArray<FEquipmentSlotEntry>& AllSlots = Equip->GetAllSlots();
		for (int32 SlotIdx = 0; SlotIdx < AllSlots.Num(); ++SlotIdx)
		{
			const FEquipmentSlotEntry& Slot = AllSlots[SlotIdx];
			FPersistedEquipmentSlot SavedSlot;
			SavedSlot.SlotType = Slot.SlotType;
			if (Slot.EquippedItem)
			{
				SavedSlot.bHasItem = true;
				CaptureItem(Slot.EquippedItem, SavedSlot.ItemData);
				//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] Equipment slot %d: %s"),
					//static_cast<int32>(Slot.SlotType), *Slot.EquippedItem->GetName());

				// Capture nested container items for equipped backpacks
				if (Slot.EquippedItem->ContainerInventory)
				{
					TArray<FPersistedItemData> Children;
					for (const TObjectPtr<UInventoryItemInstance>& CItem : Slot.EquippedItem->ContainerInventory->Items)
					{
						if (!CItem) continue;
						FPersistedItemData CData;
						CaptureItem(CItem, CData);
						Children.Add(CData);
					}
					if (Children.Num() > 0)
					{
						// Negative keys = equipment slot items: -(slotIdx + 1)
						FPersistedItemDataArray& Arr = SavedContainerItems.Add(-(SlotIdx + 1));
						Arr.Items = MoveTemp(Children);
					}
				}
			}
			else
			{
				SavedSlot.bHasItem = false;
			}
			SavedEquipmentSlots.Add(SavedSlot);
		}
	}

	bHasData = true;
	//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] Captured %d inventory items, %d equipment slots"),
		//SavedInventoryItems.Num(), SavedEquipmentSlots.Num());
}

void UInventoryPersistenceSubsystem::CaptureItem(const UInventoryItemInstance* Source, FPersistedItemData& OutData)
{
	if (!Source || !Source->ItemData) return;

	OutData.ItemDataPath = FSoftObjectPath(Source->ItemData);
	OutData.StackSize = Source->StackSize;
	OutData.Durability = Source->Durability;
	OutData.MaxDurability = Source->MaxDurability;
	OutData.SlotX = Source->SlotX;
	OutData.SlotY = Source->SlotY;
	OutData.Width = Source->Width;
	OutData.Height = Source->Height;
	OutData.ShapeMask = Source->ShapeMask;
	OutData.RotationQuarterTurns = Source->RotationQuarterTurns;
	OutData.WeaponMagazineAmmo = Source->WeaponMagazineAmmo;
	OutData.WeaponStatOverrides = Source->WeaponStatOverrides;
	OutData.WeaponModLevels = Source->WeaponModLevels;
}

// ---- Restore --------------------------------------------------------------

void UInventoryPersistenceSubsystem::RestoreToPlayer(AAlex_PlayerCharacter* Player)
{
	if (!bHasData)
	{
		//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] RestoreToPlayer — no captured data, skipping"));
		return;
	}
	if (!Player)
	{
		//UE_LOG(LogInventoryPersistence, Warning, TEXT("[Persist] RestoreToPlayer — Player is null"));
		bHasData = false;
		return;
	}

	UInventoryComponent* Inv = Player->GetInventory();
	UEquipmentComponent* Equip = Player->GetEquipment();

	if (!Inv)
	{
		//UE_LOG(LogInventoryPersistence, Warning, TEXT("[Persist] RestoreToPlayer — Inventory component is null"));
		bHasData = false;
		return;
	}

	// Clear and resize the fresh inventory
	Inv->Clear();
	Inv->InitializeGrid(
		FMath::Max(Inv->GridWidth, SavedGridWidth),
		FMath::Max(Inv->GridHeight, SavedGridHeight));

	// Restore player stats
	Player->SetMaxHealth(SavedPlayerStats.MaxHealth);
	Player->SetHealth(SavedPlayerStats.Health);
	Player->SetMaxStamina(SavedPlayerStats.MaxStamina);
	Player->SetStamina(SavedPlayerStats.Stamina);
	Player->StaminaRecoveryRate = SavedPlayerStats.StaminaRecoveryRate;
	Player->SetMaxHunger(SavedPlayerStats.MaxHunger);
	Player->SetHunger(SavedPlayerStats.Hunger);
	Player->HungerDrainRate = SavedPlayerStats.HungerDrainRate;
	Player->SetMaxHydration(SavedPlayerStats.MaxHydration);
	Player->SetHydration(SavedPlayerStats.Hydration);
	Player->HydrationDrainRate = SavedPlayerStats.HydrationDrainRate;

	//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] Restoring — Inventory %dx%d, %d items, %d equip slots, Stats HP=%.0f/%.0f"),
		//Inv->GridWidth, Inv->GridHeight, SavedInventoryItems.Num(), SavedEquipmentSlots.Num(),
		//SavedPlayerStats.Health, SavedPlayerStats.MaxHealth);

	// 1. Restore inventory items (track new instances for container lookup)
	TArray<UInventoryItemInstance*> RestoredInstances;
	RestoredInstances.SetNum(SavedInventoryItems.Num());
	for (int32 i = 0; i < SavedInventoryItems.Num(); ++i)
	{
		UInventoryItemInstance* Inst = RestoreItem(SavedInventoryItems[i], Inv);
		RestoredInstances[i] = Inst;
		if (Inst)
		{
			//UE_LOG(LogInventoryPersistence, Verbose, TEXT("[Persist] Restored inventory item '%s' at (%d,%d)"),
				//*Inst->GetName(), Inst->SlotX, Inst->SlotY);
		}
	}

	// 1b. Restore nested container-backpack items
	for (const auto& Pair : SavedContainerItems)
	{
		UInventoryItemInstance* ParentItem = nullptr;

		if (Pair.Key >= 0)
		{
			// Inventory item
			if (Pair.Key < RestoredInstances.Num())
				ParentItem = RestoredInstances[Pair.Key];
		}
		else
		{
			// Equipment item: key = -(slotIdx + 1), already equipped via EquipItemFromInventory
			const int32 SlotIdx = -(Pair.Key) - 1;
			if (Equip && SlotIdx >= 0 && SlotIdx < SavedEquipmentSlots.Num())
			{
				ParentItem = Equip->GetEquippedItem(SavedEquipmentSlots[SlotIdx].SlotType);
			}
		}

		if (!ParentItem) continue;

		if (!ParentItem->ContainerInventory)
			ParentItem->ContainerInventory = NewObject<UInventoryComponent>(ParentItem);
		if (!ParentItem->ContainerInventory) continue;

		const int32 ContSize = FMath::Max(4, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Pair.Value.Items.Num()))));
		ParentItem->ContainerInventory->InitializeGrid(ContSize, ContSize);
		for (const FPersistedItemData& CData : Pair.Value.Items)
		{
			RestoreItem(CData, ParentItem->ContainerInventory);
		}
		//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] Restored %d container items for parent key %d"),
			//Pair.Value.Items.Num(), Pair.Key);
	}

	// 2. Restore equipment (add to inventory, then equip)
	if (Equip)
	{
		for (const FPersistedEquipmentSlot& Slot : SavedEquipmentSlots)
		{
			if (!Slot.bHasItem)
				continue;

			UInventoryItemInstance* Item = RestoreItem(Slot.ItemData, Inv);
			if (Item)
			{
				Equip->EquipItemFromInventory(Inv, Item, Slot.SlotType);
				//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] Restored equipment slot %d: '%s'"),
					//static_cast<int32>(Slot.SlotType), *Item->GetName());
			}
		}
	}

	if (AAlex_PlayerController* PC = Cast<AAlex_PlayerController>(Player->GetController()))
	{
		PC->RefreshPlayerInventoryUI();
	}

	ClearCapturedData();
	//UE_LOG(LogInventoryPersistence, Log, TEXT("[Persist] Restore complete"));
}

UInventoryItemInstance* UInventoryPersistenceSubsystem::RestoreItem(
	const FPersistedItemData& Data, UInventoryComponent* TargetInv) const
{
	UInventoryItemDataAsset* DA = ResolveDataAsset(Data.ItemDataPath);
	if (!DA)
	{
		//UE_LOG(LogInventoryPersistence, Warning, TEXT("[Persist] Failed to resolve DataAsset: %s"),
			//*Data.ItemDataPath.ToString());
		return nullptr;
	}

	// Try saved position first, fall back to find-space
	int32 UseX = Data.SlotX, UseY = Data.SlotY;
	if (UseX < 0 || UseY < 0 ||
		!TargetInv->IsSpaceAvailable(Data.Width, Data.Height, Data.ShapeMask, UseX, UseY))
	{
		if (!TargetInv->FindSpaceForItem(Data.Width, Data.Height, Data.ShapeMask, UseX, UseY))
		{
			//UE_LOG(LogInventoryPersistence, Warning, TEXT("[Persist] No space for '%s' in inventory"),
				//*DA->GetName());
			return nullptr;
		}
	}

	UInventoryItemInstance* Item = TargetInv->AddItemWithShapeAtPosition(
		DA, UseX, UseY, Data.StackSize,
		Data.Width, Data.Height, Data.ShapeMask,
		Data.RotationQuarterTurns,
		Data.Durability, Data.MaxDurability);

	if (!Item) return nullptr;

	// Restore per-instance runtime state
	Item->WeaponMagazineAmmo = Data.WeaponMagazineAmmo;
	Item->WeaponStatOverrides = Data.WeaponStatOverrides;
	Item->WeaponModLevels = Data.WeaponModLevels;

	return Item;
}

UInventoryItemDataAsset* UInventoryPersistenceSubsystem::ResolveDataAsset(const FSoftObjectPath& Path)
{
	if (!Path.IsValid()) return nullptr;

	UObject* Loaded = Path.TryLoad();
	if (!Loaded) return nullptr;

	if (UInventoryItemDataAsset* Direct = Cast<UInventoryItemDataAsset>(Loaded))
		return Direct;

	// Blueprint DataAsset: get CDO
	if (UBlueprint* BP = Cast<UBlueprint>(Loaded))
	{
		if (BP->GeneratedClass && BP->GeneratedClass->IsChildOf(UInventoryItemDataAsset::StaticClass()))
			return Cast<UInventoryItemDataAsset>(BP->GeneratedClass->GetDefaultObject());
	}

	return nullptr;
}

void UInventoryPersistenceSubsystem::ClearCapturedData()
{
	SavedPlayerStats = FPersistedPlayerStats();
	SavedInventoryItems.Empty();
	SavedEquipmentSlots.Empty();
	SavedContainerItems.Empty();
	bHasData = false;
}
