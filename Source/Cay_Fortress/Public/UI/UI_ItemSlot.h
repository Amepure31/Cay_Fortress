#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemRarity.h"
#include "UI_ItemSlot.generated.h"

class UImage;
class UOverlay;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, UUI_ItemSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotHovered, UUI_ItemSlot*, Slot);

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

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlotPosition(int32 InGridX, int32 InGridY);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void GetSlotPosition(int32& OutGridX, int32& OutGridY) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetCanPlace(bool bCanPlace);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlace() const { return bCanPlaceItem; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetOccupied(bool bOccupied, EInventoryItemRarity Rarity = EInventoryItemRarity::Normal);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsOccupied() const { return bIsOccupied; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BindItem(UInventoryItemInstance* InItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnbindItem();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryItemInstance* GetBoundItem() const { return BoundItem; }

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

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

	FLinearColor GetRarityColor(EInventoryItemRarity Rarity) const;
};