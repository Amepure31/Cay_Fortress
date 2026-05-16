#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/FItemShapeMask.h"
#include "Equipment/EquipmentTypes.h"
#include "InventoryPersistenceSubsystem.generated.h"

class UInventoryItemDataAsset;

/** Serialized form of a single inventory item instance. */
USTRUCT()
struct FPersistedItemData
{
	GENERATED_BODY()

	UPROPERTY()
	FSoftObjectPath ItemDataPath;

	UPROPERTY()
	int32 StackSize = 1;
	UPROPERTY()
	int32 Durability = 0;
	UPROPERTY()
	int32 MaxDurability = 0;
	UPROPERTY()
	int32 SlotX = -1;
	UPROPERTY()
	int32 SlotY = -1;
	UPROPERTY()
	int32 Width = 1;
	UPROPERTY()
	int32 Height = 1;
	UPROPERTY()
	FFItemShapeMask ShapeMask;
	UPROPERTY()
	int32 RotationQuarterTurns = 0;
	UPROPERTY()
	int32 WeaponMagazineAmmo = 0;

	/** Workbench mod overrides on this instance. */
	UPROPERTY()
	FWeaponStatOverrides WeaponStatOverrides;

	UPROPERTY()
	TMap<uint8, int32> WeaponModLevels;
};

/** Wrapper struct needed because TMap can't hold raw TArray as value in UHT. */
USTRUCT()
struct FPersistedItemDataArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FPersistedItemData> Items;
};

/** Serialized equipment slot entry. */
USTRUCT()
struct FPersistedEquipmentSlot
{
	GENERATED_BODY()

	UPROPERTY()
	EEquipmentSlotType SlotType;

	/** Whether this slot had an item equipped. */
	UPROPERTY()
	bool bHasItem = false;

	/** The equipped item's data (valid only when bHasItem is true). */
	UPROPERTY()
	FPersistedItemData ItemData;
};

/** Serialized player character stats. */
USTRUCT()
struct FPersistedPlayerStats
{
	GENERATED_BODY()

	UPROPERTY()
	float Health = 100.0f;
	UPROPERTY()
	float MaxHealth = 100.0f;
	UPROPERTY()
	float Stamina = 100.0f;
	UPROPERTY()
	float MaxStamina = 100.0f;
	UPROPERTY()
	float StaminaRecoveryRate = 5.0f;
	UPROPERTY()
	float Hunger = 100.0f;
	UPROPERTY()
	float MaxHunger = 100.0f;
	UPROPERTY()
	float HungerDrainRate = 1.0f;
	UPROPERTY()
	float Hydration = 100.0f;
	UPROPERTY()
	float MaxHydration = 100.0f;
	UPROPERTY()
	float HydrationDrainRate = 1.0f;
};

/**
 * Captures the player's inventory and equipment before a level transition
 * and restores them afterwards. Lives on the GameInstance so it survives
 * UWorld destruction during OpenLevel / ServerTravel.
 */
UCLASS()
class UInventoryPersistenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Capture inventory + equipment FROM the given player pawn. */
	void CaptureFromPlayer(class AAlex_PlayerCharacter* Player);

	/** Restore inventory + equipment TO the given player pawn. */
	void RestoreToPlayer(class AAlex_PlayerCharacter* Player);

	/** Whether captured data is waiting to be restored. */
	bool HasCapturedData() const { return bHasData; }

	/** Discard captured data without restoring. */
	void ClearCapturedData();

private:
	static void CaptureItem(const class UInventoryItemInstance* Source, FPersistedItemData& OutData);
	class UInventoryItemInstance* RestoreItem(const FPersistedItemData& Data, class UInventoryComponent* TargetInv) const;
	static class UInventoryItemDataAsset* ResolveDataAsset(const FSoftObjectPath& Path);

	UPROPERTY()
	FPersistedPlayerStats SavedPlayerStats;

	UPROPERTY()
	TArray<FPersistedItemData> SavedInventoryItems;

	UPROPERTY()
	TArray<FPersistedEquipmentSlot> SavedEquipmentSlots;

	/** Container-backpack items, keyed by parent item's index in SavedInventoryItems. */
	UPROPERTY()
	TMap<int32, FPersistedItemDataArray> SavedContainerItems;

	UPROPERTY()
	int32 SavedGridWidth = 4;

	UPROPERTY()
	int32 SavedGridHeight = 5;

	UPROPERTY()
	int32 SavedBackpackHeightCells = 3;

	UPROPERTY()
	int32 SavedWeaponMaxWidthCells = 5;

	bool bHasData = false;
};
