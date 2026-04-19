// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/FItemShapeMask.h"
#include "UI/UI_ItemSlot.h"
#include "UI_ItemWidget.generated.h"

class UInventoryItemInstance;
class UImage;
class UTextBlock;
class UDragDropOperation;
class UUI_Inventory;
class USizeBox;
class UPanelWidget;

UCLASS()
class CAY_FORTRESS_API UUI_ItemWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual UInventoryItemInstance* GetItemInstance() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetDragGrabCellX() const { return DragGrabCellX; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetDragGrabCellY() const { return DragGrabCellY; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetDragGrabCell(int32 InCellX, int32 InCellY)
	{
		DragGrabCellX = InCellX;
		DragGrabCellY = InCellY;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void SetItemInstance(UInventoryItemInstance* InItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BeginDragSession();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EndDragSession();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RotateDragFootprintClockwise();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetCurrentDragWidth() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetCurrentDragHeight() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetCurrentDragRotationQuarterTurns() const { return DragRotationQuarterTurns; }

	const FFItemShapeMask& GetCurrentDragShapeMask() const;

	void SetActiveDragVisual(UUI_ItemWidget* InDragVisual);
	void SetActiveDragVisualHost(USizeBox* InDragVisualHost);
	void SetActiveDragOperation(UDragDropOperation* InDragOperation);
	void HideSourceVisualRootForDrag();
	void RestoreSourceVisualRootAfterDrag();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetOwningInventory(UUI_Inventory* InOwningInventory) { OwningInventory = InOwningInventory; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UUI_Inventory* GetOwningInventory() const { return OwningInventory.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void OnDraggedOver(UUI_ItemSlot* TargetSlot, bool bCanPlace);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void OnDropped(UUI_ItemSlot* TargetSlot);

	virtual void OnDragCancelled();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

public:
	bool ResolveMaskCellFromScreenPosition(const FVector2D& ScreenPosition, int32& OutCellX, int32& OutCellY) const;
	bool IsMaskCellInteractable(int32 CellX, int32 CellY) const;
	bool IsScreenPositionInteractable(const FVector2D& ScreenPosition) const;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	UImage* ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	UTextBlock* StackSizeText;

private:
	UPROPERTY()
	UInventoryItemInstance* BoundItem;

	UPROPERTY()
	int32 DragGrabCellX;

	UPROPERTY()
	int32 DragGrabCellY;

	UPROPERTY()
	TObjectPtr<UUI_Inventory> OwningInventory;

	UPROPERTY()
	bool bDragSessionActive = false;

	UPROPERTY()
	int32 DragFootprintWidth = 1;

	UPROPERTY()
	int32 DragFootprintHeight = 1;

	UPROPERTY()
	FFItemShapeMask DragFootprintMask;

	UPROPERTY()
	int32 DragRotationQuarterTurns = 0;

	UPROPERTY()
	TObjectPtr<UUI_ItemWidget> ActiveDragVisual;

	UPROPERTY()
	TObjectPtr<USizeBox> ActiveDragVisualHost;

	UPROPERTY()
	TObjectPtr<UDragDropOperation> ActiveDragOperation;

	void UpdateHoverPreviewByPointer(const FVector2D& ScreenPosition);
	void UpdateStackSizeDisplay();
	void UpdateActiveDragVisualHostSize();
	void UpdateDragOperationAnchor();
	void ApplyVisualDimensions(int32 InWidth, int32 InHeight);
	void SetDragFootprintInternal(int32 InWidth, int32 InHeight, const FFItemShapeMask& InMask, int32 InRotationQuarterTurns);

	UPROPERTY()
	TObjectPtr<UPanelWidget> SourceVisualRootBeforeDrag;
};
