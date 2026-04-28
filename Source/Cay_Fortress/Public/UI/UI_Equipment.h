#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "Equipment/EquipmentTypes.h"
#include "UI_Equipment.generated.h"

class UEquipmentComponent;
class UInventoryComponent;
class UUI_EquipmentSlot;
class UUI_ItemWidget;
class UUI_Inventory;

UCLASS()
class CAY_FORTRESS_API UUI_Equipment : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	UUI_Equipment();

	/** 背包槽位在装备栏中的显示高度（格数）。物品在背包里可更高，此处为栏位固定占位高度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Layout", meta = (ClampMin = "1", UIMin = "1"))
	int32 BackpackSlotHeightCells;

	/** 武器槽在装备栏中的最大宽度（格数），对应「宽不确定」的上限占位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Layout", meta = (ClampMin = "1", UIMin = "1"))
	int32 WeaponSlotMaxWidthCells;

	/**
	 * 未与背包 UI 绑定时使用的单格像素边长（与 UUI_Inventory::SlotSize 同语义，且不低于 64）。
	 * 为 0 时仅用 64。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Layout", meta = (ClampMin = "0", UIMin = "0"))
	int32 FallbackEquipmentCellPixelSize;

	/** 五个装备槽在 UMG 中手动放置，变量名须与蓝图一致；可为不同 Widget 蓝图（均继承 UUI_EquipmentSlot）。槽位默认背景色在槽位蓝图中设置，装备后由槽位代码按物品稀有度改色（与背包格子一致）。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UUI_EquipmentSlot> EquipmentSlot_Head;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UUI_EquipmentSlot> EquipmentSlot_Chest;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UUI_EquipmentSlot> EquipmentSlot_Backpack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UUI_EquipmentSlot> EquipmentSlot_Weapon1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UUI_EquipmentSlot> EquipmentSlot_Weapon2;

	UFUNCTION(BlueprintCallable, Category = "Equipment", meta = (AdvancedDisplay = "InInventoryUI"))
	void BindEquipment(UEquipmentComponent* InEquipment, UInventoryComponent* InPlayerInventory, UUI_Inventory* InInventoryUI = nullptr);

	/** 背包 UI 已存在时仅同步单格像素，并刷新槽位尺寸（不重建槽）。 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetInventoryUIForCellSync(UUI_Inventory* InInventoryUI);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshAllSlots();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UEquipmentComponent* GetBoundEquipment() const { return BoundEquipment; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UInventoryComponent* GetPlayerInventory() const { return PlayerInventory; }

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipSlot(EEquipmentSlotType TargetEquipmentSlot);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment|Layout")
	int32 GetEffectiveEquipmentCellPixelSize() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment|Layout")
	void GetSlotGridSize(EEquipmentSlotType SlotType, int32& OutWidthCells, int32& OutHeightCells) const;

	bool TryEquipFromDrag(UUI_ItemWidget* SourceWidget, EEquipmentSlotType TargetEquipmentSlot);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TObjectPtr<UEquipmentComponent> BoundEquipment;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> PlayerInventory;

	UPROPERTY()
	TWeakObjectPtr<UUI_Inventory> SyncedInventoryWidget;

	UPROPERTY()
	TArray<TObjectPtr<UUI_EquipmentSlot>> SlotWidgets;

	UFUNCTION()
	void OnEquipmentChanged(EEquipmentSlotType SlotType, UInventoryItemInstance* NewItem);

	void CreateSlots();
};
