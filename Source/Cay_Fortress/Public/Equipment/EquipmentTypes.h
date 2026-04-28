#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "EquipmentTypes.generated.h"

class UInventoryItemInstance;

UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	Head     UMETA(DisplayName = "头盔"),
	Chest    UMETA(DisplayName = "护甲"),
	Backpack UMETA(DisplayName = "背包"),
	Weapon1  UMETA(DisplayName = "武器栏1"),
	Weapon2  UMETA(DisplayName = "武器栏2"),

	MAX      UMETA(Hidden)
};

/**
 * 装备槽在 UI 上占用的「背包格子」尺寸（与 UUI_Inventory 的单格像素一致）。
 * 头盔 3x3，护甲 4x4，背包宽 2、高度由 BackpackHeightCells 配置，武器高 2、宽度由 WeaponMaxWidthCells 配置。
 */
FORCEINLINE void GetEquipmentSlotGridCells(
	EEquipmentSlotType Slot,
	int32 BackpackHeightCells,
	int32 WeaponMaxWidthCells,
	int32& OutWidthCells,
	int32& OutHeightCells)
{
	switch (Slot)
	{
	case EEquipmentSlotType::Head:
		OutWidthCells = 3;
		OutHeightCells = 3;
		break;
	case EEquipmentSlotType::Chest:
		OutWidthCells = 4;
		OutHeightCells = 4;
		break;
	case EEquipmentSlotType::Backpack:
		OutWidthCells = 2;
		OutHeightCells = FMath::Max(1, BackpackHeightCells);
		break;
	case EEquipmentSlotType::Weapon1:
	case EEquipmentSlotType::Weapon2:
		OutWidthCells = FMath::Max(1, WeaponMaxWidthCells);
		OutHeightCells = 2;
		break;
	default:
		OutWidthCells = 1;
		OutHeightCells = 1;
		break;
	}
}

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FEquipmentSlotEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EEquipmentSlotType SlotType;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UInventoryItemInstance> EquippedItem;

	FEquipmentSlotEntry()
		: SlotType(EEquipmentSlotType::Head)
		, EquippedItem(nullptr)
	{
	}

	explicit FEquipmentSlotEntry(EEquipmentSlotType InSlotType)
		: SlotType(InSlotType)
		, EquippedItem(nullptr)
	{
	}
};
