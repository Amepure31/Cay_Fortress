// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_ItemWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Inventory/InventoryItemRarity.h"
#include "Inventory/FInventoryItemInstance.h"
#include "UI/UI_Inventory.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "Widgets/SWidget.h"
#include "Blueprint/DragDropOperation.h"

UInventoryItemInstance* UUI_ItemWidget::GetItemInstance() const
{
	return BoundItem;
}

void UUI_ItemWidget::SetItemInstance(UInventoryItemInstance* InItemInstance)
{
	BoundItem = InItemInstance;
	if (!InItemInstance || !InItemInstance->ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemWidget][Bind] Invalid item instance or item data."));
		return;
	}

	const FString ItemName = InItemInstance->ItemData->ItemData.ItemName.IsEmpty()
		? InItemInstance->GetName()
		: InItemInstance->ItemData->ItemData.ItemName.ToString();
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[ItemWidget][Bind] Item='%s' GridSize=%dx%d MaskSize=%dx%d"),
		*ItemName,
		InItemInstance->Width,
		InItemInstance->Height,
		InItemInstance->ShapeMask.Width,
		InItemInstance->ShapeMask.Height
	);

	if (InItemInstance && InItemInstance->ItemData)
	{
		if (ItemIcon)
		{
			UTexture2D* IconTexture = InItemInstance->ItemData->ItemData.Icon;
			ItemIcon->SetBrushFromTexture(IconTexture);
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemWidget][Bind] Item='%s' Texture='%s' TextureSize=%dx%d"),
				*ItemName,
				*GetNameSafe(IconTexture),
				IconTexture ? IconTexture->GetSizeX() : 0,
				IconTexture ? IconTexture->GetSizeY() : 0
			);

			// Keep icon aspect ratio aligned with item grid dimensions (e.g. 1x2, 3x2),
			// so different items can share one widget class without inheriting a fixed visual ratio.
			const float AspectWidth = static_cast<float>(FMath::Max(1, InItemInstance->Width));
			const float AspectHeight = static_cast<float>(FMath::Max(1, InItemInstance->Height));
			const FVector2D DesiredIconSize = FVector2D(AspectWidth, AspectHeight) * 100.0f;
			ItemIcon->SetDesiredSizeOverride(DesiredIconSize);

			FSlateBrush IconBrush = ItemIcon->GetBrush();
			IconBrush.SetImageSize(DesiredIconSize);
			ItemIcon->SetBrush(IconBrush);
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemWidget][Bind] Item='%s' DesiredIconSize=(%.1f,%.1f) BrushImageSize=(%.1f,%.1f)"),
				*ItemName,
				DesiredIconSize.X,
				DesiredIconSize.Y,
				IconBrush.ImageSize.X,
				IconBrush.ImageSize.Y
			);
		}
		UpdateStackSizeDisplay();
	}
}

void UUI_ItemWidget::OnDraggedOver(UUI_ItemSlot* TargetSlot, bool bCanPlace)
{
	if (TargetSlot)
	{
		TargetSlot->SetCanPlace(bCanPlace);
	}
}

void UUI_ItemWidget::OnDropped(UUI_ItemSlot* TargetSlot)
{
	if (TargetSlot)
	{
		TargetSlot->SetCanPlace(false);
	}
}

void UUI_ItemWidget::OnDragCancelled()
{
	if (BoundItem && BoundItem->SlotX >= 0 && BoundItem->SlotY >= 0)
	{
		if (UUI_Inventory* InventoryWidget = OwningInventory.Get())
		{
			if (InventoryWidget->GetBoundInventory())
			{
				InventoryWidget->SetDraggedItemWidget(nullptr);
			}
		}
	}
}

void UUI_ItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Keep BoundItem assigned by SetItemInstance; do not reset here.
}

void UUI_ItemWidget::NativeDestruct()
{
	BoundItem = nullptr;
	OwningInventory = nullptr;
	Super::NativeDestruct();
}

FReply UUI_ItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (!BoundItem || !BoundItem->ItemData)
		{
			return FReply::Unhandled();
		}

		int32 HitCellX = INDEX_NONE;
		int32 HitCellY = INDEX_NONE;
		const bool bResolvedToGridCell = ResolveMaskCellFromScreenPosition(InMouseEvent.GetScreenSpacePosition(), HitCellX, HitCellY);
		const bool bHitInteractableMaskCell = bResolvedToGridCell && IsMaskCellInteractable(HitCellX, HitCellY);
		if (!bHitInteractableMaskCell)
		{
			// Consume clicks on non-interactable mask area to prevent fallback drag/click from parent slot.
			return FReply::Handled();
		}

		DragGrabCellX = HitCellX;
		DragGrabCellY = HitCellY;

		if (TSharedPtr<SWidget> WidgetPtr = GetCachedWidget())
		{
			return FReply::Handled().DetectDrag(WidgetPtr.ToSharedRef(), EKeys::LeftMouseButton);
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UUI_ItemWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsScreenPositionInteractable(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply UUI_ItemWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UpdateHoverPreviewByPointer(InMouseEvent.GetScreenSpacePosition());
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UUI_ItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	UpdateHoverPreviewByPointer(InMouseEvent.GetScreenSpacePosition());
}

void UUI_ItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (UUI_Inventory* InventoryWidget = OwningInventory.Get())
	{
		InventoryWidget->ClearItemHoverPreview(BoundItem);
	}
}

FReply UUI_ItemWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!IsScreenPositionInteractable(InGestureEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

void UUI_ItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!BoundItem || !BoundItem->ItemData)
	{
		return;
	}

	UDragDropOperation* DragOperation = NewObject<UDragDropOperation>(this);
	if (!DragOperation)
	{
		return;
	}

	UUI_ItemWidget* DragVisual = CreateWidget<UUI_ItemWidget>(GetWorld(), GetClass());
	if (DragVisual)
	{
		DragVisual->SetItemInstance(BoundItem);
		DragVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	DragOperation->Payload = this;
	DragOperation->DefaultDragVisual = DragVisual;
	DragOperation->Pivot = EDragPivot::MouseDown;
	OutOperation = DragOperation;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UUI_Inventory* InventoryWidget = OwningInventory.Get())
	{
		InventoryWidget->SetDraggedItemWidget(this);
	}
}

void UUI_ItemWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	if (UUI_Inventory* InventoryWidget = OwningInventory.Get())
	{
		InventoryWidget->ClearPlacementPreview();
		InventoryWidget->UpdateInventory();
		InventoryWidget->SetDraggedItemWidget(nullptr);
	}
	SetVisibility(ESlateVisibility::Visible);
}

void UUI_ItemWidget::UpdateHoverPreviewByPointer(const FVector2D& ScreenPosition)
{
	UUI_Inventory* InventoryWidget = OwningInventory.Get();
	if (!InventoryWidget || !BoundItem)
	{
		return;
	}

	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	const bool bResolved = ResolveMaskCellFromScreenPosition(ScreenPosition, CellX, CellY);
	const bool bInteractable = bResolved && IsMaskCellInteractable(CellX, CellY);
	if (bInteractable)
	{
		InventoryWidget->SetItemHoverPreview(BoundItem);
	}
	else
	{
		InventoryWidget->ClearItemHoverPreview(BoundItem);
	}
}

void UUI_ItemWidget::UpdateStackSizeDisplay()
{
	if (!BoundItem || !BoundItem->ItemData || !StackSizeText)
	{
		return;
	}

	const FInventoryItemData& ItemData = BoundItem->ItemData->ItemData;
	if (ItemData.bCanStack && BoundItem->StackSize > 1)
	{
		StackSizeText->SetText(FText::Format(FText::FromString(TEXT("{0}")), BoundItem->StackSize));
		StackSizeText->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		StackSizeText->SetVisibility(ESlateVisibility::Hidden);
	}
}

bool UUI_ItemWidget::ResolveMaskCellFromScreenPosition(const FVector2D& ScreenPosition, int32& OutCellX, int32& OutCellY) const
{
	if (!BoundItem)
	{
		return false;
	}

	const int32 ItemWidth = FMath::Max(1, BoundItem->Width);
	const int32 ItemHeight = FMath::Max(1, BoundItem->Height);

	// Use pure grid-based mapping to avoid per-pixel drift inside a single slot.
	if (const UUI_Inventory* InventoryWidget = OwningInventory.Get())
	{
		if (UUI_ItemSlot* HoveredSlot = InventoryWidget->FindSlotAtScreenPosition(ScreenPosition))
		{
			const int32 CellX = HoveredSlot->GetGridX() - BoundItem->SlotX;
			const int32 CellY = HoveredSlot->GetGridY() - BoundItem->SlotY;
			if (CellX >= 0 && CellX < ItemWidth && CellY >= 0 && CellY < ItemHeight)
			{
				OutCellX = CellX;
				OutCellY = CellY;
				return true;
			}
		}
	}
	return false;
}

bool UUI_ItemWidget::IsMaskCellInteractable(int32 CellX, int32 CellY) const
{
	if (!BoundItem)
	{
		return false;
	}

	const int32 ItemWidth = FMath::Max(1, BoundItem->Width);
	const int32 ItemHeight = FMath::Max(1, BoundItem->Height);
	if (CellX < 0 || CellX >= ItemWidth || CellY < 0 || CellY >= ItemHeight)
	{
		return false;
	}

	// No mask data means full rectangle is valid.
	if (BoundItem->ShapeMask.Width <= 0 || BoundItem->ShapeMask.Height <= 0)
	{
		return true;
	}

	return BoundItem->ShapeMask.IsOccupied(CellX, CellY);
}

bool UUI_ItemWidget::IsScreenPositionInteractable(const FVector2D& ScreenPosition) const
{
	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	if (!ResolveMaskCellFromScreenPosition(ScreenPosition, CellX, CellY))
	{
		return false;
	}
	return IsMaskCellInteractable(CellX, CellY);
}

