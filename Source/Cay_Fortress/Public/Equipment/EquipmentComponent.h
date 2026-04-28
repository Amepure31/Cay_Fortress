#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "EquipmentComponent.generated.h"

class UInventoryComponent;
class UInventoryItemInstance;
class UInventoryItemDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, EEquipmentSlotType, SlotType, UInventoryItemInstance*, NewItem);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CAY_FORTRESS_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool CanEquipItemToSlot(UInventoryItemDataAsset* ItemData, EEquipmentSlotType Slot) const;

	/**
	 * 从背包装备到指定槽位。如槽位已有装备则先卸下到背包。
	 * @return 是否装备成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItemFromInventory(UInventoryComponent* SourceInventory, UInventoryItemInstance* Item, EEquipmentSlotType Slot);

	/**
	 * 将指定槽位装备卸下到背包
	 * @return 是否卸下成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipItemToInventory(EEquipmentSlotType Slot, UInventoryComponent* TargetInventory);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UInventoryItemInstance* GetEquippedItem(EEquipmentSlotType Slot) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	bool IsSlotOccupied(EEquipmentSlotType Slot) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	const TArray<FEquipmentSlotEntry>& GetAllSlots() const { return Slots; }

	/** 聚合所有已装备护甲的总减伤比例（叠加，钳位 0~1） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	float GetTotalDamageReduction() const;

	/** 聚合所有已装备护甲的总护甲值 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	float GetTotalArmorValue() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FEquipmentSlotEntry> Slots;

	FEquipmentSlotEntry* FindSlotEntry(EEquipmentSlotType Slot);
	const FEquipmentSlotEntry* FindSlotEntry(EEquipmentSlotType Slot) const;

	static bool IsArmorSlot(EEquipmentSlotType Slot);
	static bool IsWeaponSlot(EEquipmentSlotType Slot);
	static EEquipmentSlotType ArmorEquipSlotToEquipmentSlot(uint8 ArmorSlot);
};
