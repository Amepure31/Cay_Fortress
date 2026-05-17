#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemRarity.h"
#include "UI_ItemSlot.generated.h"

class UImage;
class UOverlay;
class UUI_ItemWidget;
class UDragDropOperation;
class UUI_Inventory;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, UUI_ItemSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotHovered, UUI_ItemSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotRightClicked, UUI_ItemSlot*, Slot, FVector2D, ScreenPosition);

UCLASS()
class CAY_FORTRESS_API UUI_ItemSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UImage* Background;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UImage* HoverOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UImage* CanPlaceOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UImage* CannotPlaceOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UOverlay* ItemContainer;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnSlotClicked OnSlotClicked;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnSlotHovered OnSlotHovered;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnSlotRightClicked OnSlotRightClicked;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlotPosition(int32 InGridX, int32 InGridY);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void GetSlotPosition(int32& OutGridX, int32& OutGridY) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetCanPlace(bool bCanPlace);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearCanPlacePreview();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetHoverActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlace() const { return bCanPlaceItem; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateCanPlaceFromItem(UUI_ItemWidget* ItemWidget);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetOccupied(bool bOccupied, EInventoryItemRarity Rarity = EInventoryItemRarity::Normal);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsOccupied() const { return bIsOccupied; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BindItem(UInventoryItemInstance* InItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnbindItem();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemWidgetClass(TSubclassOf<UUI_ItemWidget> InItemWidgetClass);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetOwningInventory(UUI_Inventory* InOwningInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryItemInstance* GetBoundItem() const { return BoundItem; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetGridX() const { return GridX; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetGridY() const { return GridY; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	UInventoryItemInstance* BoundItem;

	UPROPERTY()
	int32 GridX;

	UPROPERTY()
	int32 GridY;

	UPROPERTY()
	bool bCanPlaceItem;

	UPROPERTY()
	bool bIsOccupied;

	UPROPERTY()
	TObjectPtr<UUI_ItemWidget> ItemWidgetInstance;

	UPROPERTY()
	TSubclassOf<UUI_ItemWidget> ItemWidgetClass;

	UPROPERTY()
	TObjectPtr<UUI_Inventory> OwningInventory;

	FLinearColor GetRarityColor(EInventoryItemRarity Rarity) const;
};