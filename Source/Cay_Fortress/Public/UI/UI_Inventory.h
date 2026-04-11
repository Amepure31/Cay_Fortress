#pragma once

#include "CoreMinimal.h"
#include "UI/UI_Base_Class.h"
#include "Inventory/InventoryComponent.h"
#include "UI_Inventory.generated.h"

class UUniformGridPanel;
class UUI_ItemSlot;
class UUI_ItemTooltip;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSlotClicked, UUI_ItemSlot*, ItemSlot);

UCLASS()
class CAY_FORTRESS_API UUI_Inventory : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Settings")
	TSubclassOf<UUI_ItemSlot> ItemSlotClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Settings")
	TSubclassOf<UUI_ItemTooltip> TooltipClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UUniformGridPanel* GridPanel;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridWidth;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridHeight;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "1", UIMin = "1"))
	int32 SlotSize;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "0", UIMin = "0"))
	int32 SlotSpacing;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Events")
	FOnItemSlotClicked OnItemSlotClicked;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BindInventory(UInventoryComponent* InInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryComponent* GetBoundInventory() const { return BoundInventory; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	UInventoryComponent* BoundInventory;

	UPROPERTY()
	TArray<UUI_ItemSlot*> ItemSlots;

	UPROPERTY()
	UUI_ItemTooltip* ActiveTooltip;

	void CreateGrid();
	void OnItemSlotClickedInternal(UUI_ItemSlot* Slot);
	void ShowTooltip(UUI_ItemSlot* Slot);
	void HideTooltip();
};