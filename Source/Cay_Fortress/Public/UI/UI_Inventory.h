#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UI/UI_Base_Class.h"
#include "Inventory/InventoryComponent.h"
#include "UI/UI_ItemWidget.h"
#include "SlateBasics.h"
#include "UI_Inventory.generated.h"

class UUniformGridPanel;
class UUI_ItemSlot;
class UUI_ItemTooltip;
class UUI_ItemWidget;
class UInventoryItemDataAsset;
class UButton;
class UComboBoxString;
class UDragDropOperation;
class USizeBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSlotClicked, UUI_ItemSlot*, ItemSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContainerOpenRequested, UInventoryItemInstance*, ContainerItem);

UCLASS()
class CAY_FORTRESS_API UUI_Inventory : public UUI_Base_Class
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Settings")
	TSubclassOf<UUI_ItemSlot> ItemSlotClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Settings")
	TSubclassOf<UUI_ItemTooltip> TooltipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Settings")
	TSubclassOf<UUI_ItemWidget> ItemWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UUniformGridPanel* GridPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	UButton* AddItemButton;

	// Place this ComboBox near AddItemButton in the UMG blueprint.
	// It is hidden by default and shown when AddItemButton is clicked.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	UComboBoxString* AddItemComboBox;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridWidth;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridHeight;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "1", UIMin = "1"))
	int32 SlotSize;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Settings", meta = (ClampMin = "0", UIMin = "0"))
	int32 SlotSpacing;

	/** 背包内鼠标悬停武器时，按住此键达到时长后清空弹匣，子弹以散装形式放入背包（与拖拽时 R 旋转占位互不冲突）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon")
	FKey WeaponUnloadMagazineKey = EKeys::R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon", meta = (ClampMin = "0.1", ClampMax = "3.0", UIMin = "0.1", UIMax = "3.0"))
	float WeaponUnloadMagazineHoldSeconds = 0.45f;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Events")
	FOnItemSlotClicked OnItemSlotClicked;

	/** 悬停容器背包物品时按 R 广播 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnContainerOpenRequested OnContainerOpenRequested;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BindInventory(UInventoryComponent* InInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryComponent* GetBoundInventory() const { return BoundInventory; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	class UInventoryItemInstance* GetHoveredItemInstance() const { return HoveredItemInstance; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateSlotCanPlacePreviews(UUI_ItemWidget* ItemWidget);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetDraggedItemWidget(UUI_ItemWidget* InWidget);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UUI_ItemWidget* GetDraggedItemWidget() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetEffectiveSlotSize() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UUI_ItemSlot* GetSlotAtGrid(int32 GridX, int32 GridY) const;

	UUI_ItemSlot* FindSlotAtScreenPosition(const FVector2D& ScreenPosition) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdatePlacementPreview(class UUI_ItemWidget* ItemWidget, int32 OriginSlotX, int32 OriginSlotY);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearPlacementPreview();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemHoverPreview(class UInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearItemHoverPreview(class UInventoryItemInstance* ItemInstance = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryPlaceDraggedItem(class UUI_ItemWidget* ItemWidget, int32 OriginSlotX, int32 OriginSlotY);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItemFromDataAsset(class UInventoryItemDataAsset* ItemData, int32 StackSize = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<class UInventoryItemDataAsset*> GetAvailableItemDataAssets() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<UInventoryItemDataAsset*> GetAllItemDataAssets() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnAddItemButtonClicked();

	UFUNCTION()
	void OnAddItemSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void SuppressDraggedItemSlotVisuals(class UInventoryItemInstance* ItemInstance);
	void ShowTooltipForItem(class UInventoryItemInstance* ItemInstance);
	void UpdateHoverStateFromCursor();
	bool TryDiscardDraggedItem(class UUI_ItemWidget* ItemWidget, const FGeometry& InventoryGeometry, const FVector2D& ScreenPosition);
	bool IsDropInDiscardZone(const FGeometry& InventoryGeometry, const FVector2D& ScreenPosition) const;

	UPROPERTY()
	UInventoryComponent* BoundInventory;

	UPROPERTY()
	TArray<UUI_ItemSlot*> ItemSlots;

	UPROPERTY()
	UUI_ItemTooltip* ActiveTooltip;

	UPROPERTY()
	UUI_ItemWidget* DraggedItemWidget;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UInventoryItemDataAsset>> AddItemOptionMap;

	UPROPERTY()
	TObjectPtr<class UInventoryItemInstance> HoveredItemInstance;

	UPROPERTY(Transient)
	FVector2D LastHoverCursorPosition;

	UPROPERTY(Transient)
	bool bHasLastHoverCursorPosition = false;

	bool bWeaponUnloadKeyHeld = false;
	float WeaponUnloadKeyHoldSeconds = 0.f;
	bool bTriggeredWeaponUnloadThisHold = false;

protected:
	void CreateGrid();
	void SetSlotSize();
	void UpdateGridPanelSize();

	/** AddDynamic 要求 UFUNCTION；勿删。 */
	UFUNCTION()
	void OnItemSlotClickedInternal(UUI_ItemSlot* ClickedSlot);

	UFUNCTION()
	void ShowTooltip(UUI_ItemSlot* HoveredSlot);

	void HideTooltip();
	void UpdateTooltipPosition();
	void RefreshAddItemOptions();
	void SetAddItemListVisible(bool bVisible);
};
