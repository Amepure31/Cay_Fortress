#include "UI/UI_ItemSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Inventory/InventoryItemRarity.h"
#include "UI/UI_ItemWidget.h"
#include "UI/UI_Inventory.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Input/Reply.h"
#include "SlateBasics.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/DragDropOperation.h"

namespace
{
static constexpr float MinItemCellPixelSize = 64.0f;

static UWidget* CreateConstrainedDragVisual(
	UObject* Outer,
	UUI_ItemWidget* DragVisual,
	const UUI_Inventory* InventoryWidget,
	int32 FootprintWidth,
	int32 FootprintHeight)
{
	if (!DragVisual || !Outer)
	{
		return DragVisual;
	}

	float CellPixelSize = MinItemCellPixelSize;
	if (InventoryWidget)
	{
		CellPixelSize = static_cast<float>(InventoryWidget->GetEffectiveSlotSize());
	}

	USizeBox* DragHostSizeBox = NewObject<USizeBox>(Outer);
	if (!DragHostSizeBox)
	{
		return DragVisual;
	}

	DragHostSizeBox->SetWidthOverride(static_cast<float>(FMath::Max(1, FootprintWidth)) * CellPixelSize);
	DragHostSizeBox->SetHeightOverride(static_cast<float>(FMath::Max(1, FootprintHeight)) * CellPixelSize);
	DragHostSizeBox->AddChild(DragVisual);
	return DragHostSizeBox;
}
}

void UUI_ItemSlot::NativeConstruct()
{
	Super::NativeConstruct();
	bCanPlaceItem = false;
	bIsOccupied = false;
	ItemWidgetInstance = nullptr;
	if (!ItemWidgetClass)
	{
		ItemWidgetClass = UUI_ItemWidget::StaticClass();
	}

	if (HoverOverlay)
	{
		HoverOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CanPlaceOverlay)
	{
		CanPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CannotPlaceOverlay)
	{
		CannotPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::NativeDestruct()
{
	if (ItemWidgetInstance)
	{
		ItemWidgetInstance->RemoveFromParent();
		ItemWidgetInstance = nullptr;
	}

	Super::NativeDestruct();
}

FReply UUI_ItemSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	OnSlotClicked.Broadcast(this);

	if (BoundItem && InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnSlotRightClicked.Broadcast(this, InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	if (BoundItem && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const int32 CellX = GridX - BoundItem->SlotX;
		const int32 CellY = GridY - BoundItem->SlotY;
		const bool bInsideItemRect =
			CellX >= 0 && CellX < FMath::Max(1, BoundItem->Width) &&
			CellY >= 0 && CellY < FMath::Max(1, BoundItem->Height);
		const bool bMaskAllows =
			(BoundItem->ShapeMask.Width <= 0 || BoundItem->ShapeMask.Height <= 0) ||
			BoundItem->ShapeMask.IsOccupied(CellX, CellY);

		if (!bInsideItemRect || !bMaskAllows)
		{
			return FReply::Unhandled();
		}

		if (TSharedPtr<SWidget> WidgetPtr = GetCachedWidget())
		{
			return FReply::Handled().DetectDrag(WidgetPtr.ToSharedRef(), EKeys::LeftMouseButton);
		}
		return FReply::Handled();
	}

	return FReply::Handled();
}

void UUI_ItemSlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!BoundItem || !BoundItem->ItemData)
	{
		return;
	}

	UUI_ItemWidget* SourceItemWidget = ItemWidgetInstance;
	if (!SourceItemWidget)
	{
		if (!ItemWidgetClass)
		{
			return;
		}
		SourceItemWidget = CreateWidget<UUI_ItemWidget>(GetWorld(), ItemWidgetClass);
		if (!SourceItemWidget)
		{
			return;
		}
		SourceItemWidget->SetOwningInventory(OwningInventory);
		SourceItemWidget->SetItemInstance(BoundItem);
	}

	const int32 ItemWidth = FMath::Max(1, BoundItem->Width);
	const int32 ItemHeight = FMath::Max(1, BoundItem->Height);
	int32 GrabCellX = GridX - BoundItem->SlotX;
	int32 GrabCellY = GridY - BoundItem->SlotY;

	if (GrabCellX < 0 || GrabCellX >= ItemWidth || GrabCellY < 0 || GrabCellY >= ItemHeight)
	{
		OutOperation = nullptr;
		return;
	}
	if (!SourceItemWidget->IsMaskCellInteractable(GrabCellX, GrabCellY))
	{
		OutOperation = nullptr;
		return;
	}

	SourceItemWidget->SetDragGrabCell(GrabCellX, GrabCellY);
	SourceItemWidget->BeginDragSession();

	UDragDropOperation* DragOperation = NewObject<UDragDropOperation>(this);
	if (!DragOperation)
	{
		return;
	}

	UUI_ItemWidget* DragVisual = CreateWidget<UUI_ItemWidget>(GetWorld(), SourceItemWidget->GetClass());
	UWidget* DragVisualHost = DragVisual;
	if (DragVisual)
	{
		DragVisual->SetOwningInventory(OwningInventory);
		DragVisual->SetItemInstance(BoundItem);
		DragVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
		SourceItemWidget->SetActiveDragVisual(DragVisual);
		DragVisualHost = CreateConstrainedDragVisual(
			this,
			DragVisual,
			OwningInventory,
			SourceItemWidget->GetCurrentDragWidth(),
			SourceItemWidget->GetCurrentDragHeight()
		);
		SourceItemWidget->SetActiveDragVisualHost(Cast<USizeBox>(DragVisualHost));
	}

	DragOperation->Payload = SourceItemWidget;
	DragOperation->DefaultDragVisual = DragVisualHost;
	SourceItemWidget->SetActiveDragOperation(DragOperation);
	OutOperation = DragOperation;
	SourceItemWidget->HideSourceVisualRootForDrag();
	SourceItemWidget->SetVisibility(ESlateVisibility::Hidden);
	if (OwningInventory)
	{
		OwningInventory->SetDraggedItemWidget(SourceItemWidget);
	}
}

bool UUI_ItemSlot::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);

	UUI_ItemWidget* SourceItemWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	UInventoryItemInstance* ItemInstance = SourceItemWidget ? SourceItemWidget->GetItemInstance() : nullptr;
	if (!ItemInstance)
	{
		return false;
	}

	if (OwningInventory)
	{
		UUI_ItemSlot* HoveredSlot = OwningInventory->FindSlotAtScreenPosition(InDragDropEvent.GetScreenSpacePosition());
		if (!HoveredSlot)
		{
			OwningInventory->ClearPlacementPreview();
			return true;
		}

		const int32 TargetOriginX = HoveredSlot->GetGridX() - SourceItemWidget->GetDragGrabCellX();
		const int32 TargetOriginY = HoveredSlot->GetGridY() - SourceItemWidget->GetDragGrabCellY();
		OwningInventory->UpdatePlacementPreview(SourceItemWidget, TargetOriginX, TargetOriginY);
		return true;
	}

	return false;
}

void UUI_ItemSlot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	if (OwningInventory)
	{
		OwningInventory->ClearPlacementPreview();
	}
}

void UUI_ItemSlot::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	if (OwningInventory)
	{
		OwningInventory->ClearPlacementPreview();
		OwningInventory->UpdateInventory();
		OwningInventory->SetDraggedItemWidget(nullptr);
	}
	if (ItemWidgetInstance)
	{
		ItemWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
}

bool UUI_ItemSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	UUI_ItemWidget* SourceItemWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	UInventoryItemInstance* ItemInstance = SourceItemWidget ? SourceItemWidget->GetItemInstance() : nullptr;
	if (!ItemInstance)
	{
		return false;
	}

	if (OwningInventory)
	{
		UUI_ItemSlot* HoveredSlot = OwningInventory->FindSlotAtScreenPosition(InDragDropEvent.GetScreenSpacePosition());
		if (!HoveredSlot)
		{
			OwningInventory->ClearPlacementPreview();
			OwningInventory->SetDraggedItemWidget(nullptr);
			return false;
		}

		const int32 TargetOriginX = HoveredSlot->GetGridX() - SourceItemWidget->GetDragGrabCellX();
		const int32 TargetOriginY = HoveredSlot->GetGridY() - SourceItemWidget->GetDragGrabCellY();
		const bool bPlaced = OwningInventory->TryPlaceDraggedItem(SourceItemWidget, TargetOriginX, TargetOriginY);
		OwningInventory->SetDraggedItemWidget(nullptr);
		return bPlaced;
	}

	return false;
}

void UUI_ItemSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	UInventoryItemInstance* HoverItem = BoundItem;
	if (!HoverItem && OwningInventory && OwningInventory->GetBoundInventory())
	{
		// Non-origin occupied cells don't bind item widget; resolve by grid position.
		HoverItem = OwningInventory->GetBoundInventory()->GetItemAtPosition(GridX, GridY);
	}

	if (HoverItem && OwningInventory)
	{
		OwningInventory->SetItemHoverPreview(HoverItem);
	}
	else
	{
		SetHoverActive(true);
	}

	OnSlotHovered.Broadcast(this);
}

void UUI_ItemSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	UInventoryItemInstance* HoverItem = BoundItem;
	if (!HoverItem && OwningInventory && OwningInventory->GetBoundInventory())
	{
		HoverItem = OwningInventory->GetBoundInventory()->GetItemAtPosition(GridX, GridY);
	}

	if (HoverItem && OwningInventory)
	{
		OwningInventory->ClearItemHoverPreview(HoverItem);
	}
	else
	{
		SetHoverActive(false);
	}
}

void UUI_ItemSlot::SetSlotPosition(int32 InGridX, int32 InGridY)
{
	GridX = InGridX;
	GridY = InGridY;
}

void UUI_ItemSlot::GetSlotPosition(int32& OutGridX, int32& OutGridY) const
{
	OutGridX = GridX;
	OutGridY = GridY;
}

void UUI_ItemSlot::SetCanPlace(bool bCanPlace)
{
	bCanPlaceItem = bCanPlace;

	if (CanPlaceOverlay)
	{
		CanPlaceOverlay->SetVisibility(bCanPlace ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	if (CannotPlaceOverlay)
	{
		CannotPlaceOverlay->SetVisibility(!bCanPlace ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::ClearCanPlacePreview()
{
	bCanPlaceItem = false;
	if (CanPlaceOverlay)
	{
		CanPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CannotPlaceOverlay)
	{
		CannotPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::SetHoverActive(bool bActive)
{
	if (HoverOverlay)
	{
		HoverOverlay->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::UpdateCanPlaceFromItem(UUI_ItemWidget* ItemWidget)
{
	if (!ItemWidget)
	{
		SetCanPlace(false);
		return;
	}

	UInventoryItemInstance* DraggableItem = ItemWidget->GetItemInstance();
	if (!DraggableItem)
	{
		SetCanPlace(false);
		return;
	}

	if (!BoundItem)
	{
		SetCanPlace(true);
		return;
	}

	int32 SlotX = DraggableItem->SlotX;
	int32 SlotY = DraggableItem->SlotY;

	int32 DraggableWidth = DraggableItem->Width;
	int32 DraggableHeight = DraggableItem->Height;

	int32 ItemGridX = GridX - SlotX;
	int32 ItemGridY = GridY - SlotY;

	bool bCanPlace = true;

	if (ItemGridX < 0 || ItemGridX >= DraggableWidth ||
		ItemGridY < 0 || ItemGridY >= DraggableHeight)
	{
		bCanPlace = false;
	}

	if (bCanPlace && DraggableItem->ShapeMask.Width > 0 && DraggableItem->ShapeMask.Height > 0)
	{
		if (!DraggableItem->ShapeMask.IsOccupied(ItemGridX, ItemGridY))
		{
			bCanPlace = false;
		}
	}

	SetCanPlace(bCanPlace);
}

void UUI_ItemSlot::SetOccupied(bool bOccupied, EInventoryItemRarity Rarity)
{
	bIsOccupied = bOccupied;

	if (Background)
	{
		if (bOccupied)
		{
			Background->SetColorAndOpacity(GetRarityColor(Rarity));
		}
		else
		{
			Background->SetColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f));
		}
	}

	if (CanPlaceOverlay)
	{
		CanPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CannotPlaceOverlay)
	{
		CannotPlaceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UUI_ItemSlot::BindItem(UInventoryItemInstance* InItemInstance)
{
	BoundItem = InItemInstance;

	if (ItemContainer)
	{
		ItemContainer->ClearChildren();
		ItemContainer->SetVisibility(ESlateVisibility::Visible);
		ItemContainer->SetIsEnabled(true);
	}
	ItemWidgetInstance = nullptr;

	if (InItemInstance && InItemInstance->ItemData)
	{
		const bool bOriginOccupied =
			(InItemInstance->ShapeMask.Width <= 0 || InItemInstance->ShapeMask.Height <= 0) ||
			InItemInstance->ShapeMask.IsOccupied(0, 0);
		if (bOriginOccupied)
		{
			SetOccupied(true, InItemInstance->ItemData->ItemData.Rarity);
		}

		if (ItemContainer)
		{
			if (!ItemWidgetClass)
			{
				return;
			}

			UUI_ItemWidget* NewItemWidget = CreateWidget<UUI_ItemWidget>(GetWorld(), ItemWidgetClass);
			if (NewItemWidget)
			{
				NewItemWidget->SetVisibility(ESlateVisibility::Visible);
				NewItemWidget->SetIsEnabled(true);
				NewItemWidget->SetOwningInventory(OwningInventory);
				NewItemWidget->SetItemInstance(InItemInstance);
				const FVector2D SlotSize = GetCachedGeometry().GetLocalSize();
				const float FallbackSlotSize = OwningInventory
					? static_cast<float>(OwningInventory->GetEffectiveSlotSize())
					: 64.0f;
				const float BaseWidth = SlotSize.X > 1.0f ? SlotSize.X : FallbackSlotSize;
				const float BaseHeight = SlotSize.Y > 1.0f ? SlotSize.Y : FallbackSlotSize;
				const float WidgetWidth = BaseWidth * FMath::Max(1, InItemInstance->Width);
				const float WidgetHeight = BaseHeight * FMath::Max(1, InItemInstance->Height);

				USizeBox* ItemSizeBox = NewObject<USizeBox>(this);
				if (ItemSizeBox)
				{
					ItemSizeBox->SetWidthOverride(WidgetWidth);
					ItemSizeBox->SetHeightOverride(WidgetHeight);

					UCanvasPanel* ItemVisualCanvas = NewObject<UCanvasPanel>(this);
					if (ItemVisualCanvas)
					{
						if (UCanvasPanelSlot* ItemWidgetCanvasSlot = ItemVisualCanvas->AddChildToCanvas(NewItemWidget))
						{
							// Keep widget bounds locked to the occupied footprint size.
							// This prevents 90-degree rotated visuals from expanding outside the grid area.
							ItemWidgetCanvasSlot->SetAutoSize(false);
							ItemWidgetCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
							ItemWidgetCanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
							ItemWidgetCanvasSlot->SetPosition(FVector2D::ZeroVector);
							ItemWidgetCanvasSlot->SetSize(FVector2D(WidgetWidth, WidgetHeight));
						}

						// For placed items, draw stack count on the runtime item canvas.
						// The canvas lives inside ItemSizeBox, so bottom-right stays inside item bounds.
						if (InItemInstance->StackSize > 1)
						{
							UTextBlock* SourceStackText = NewItemWidget->StackSizeText;
							if (SourceStackText)
							{
								SourceStackText->SetVisibility(ESlateVisibility::Hidden);
							}

							UTextBlock* RuntimeStackText = NewObject<UTextBlock>(this);
							if (RuntimeStackText)
							{
								RuntimeStackText->SetText(FText::AsNumber(InItemInstance->StackSize));
								RuntimeStackText->SetJustification(ETextJustify::Right);

								if (SourceStackText)
								{
									RuntimeStackText->SetFont(SourceStackText->GetFont());
									RuntimeStackText->SetColorAndOpacity(SourceStackText->GetColorAndOpacity());
									RuntimeStackText->SetShadowOffset(SourceStackText->GetShadowOffset());
									RuntimeStackText->SetShadowColorAndOpacity(SourceStackText->GetShadowColorAndOpacity());
								}
								else
								{
									RuntimeStackText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
								}

								if (UCanvasPanelSlot* StackCanvasSlot = ItemVisualCanvas->AddChildToCanvas(RuntimeStackText))
								{
									StackCanvasSlot->SetAutoSize(true);
									StackCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
									StackCanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
									StackCanvasSlot->SetPosition(FVector2D(WidgetWidth - 4.0f, WidgetHeight - 4.0f));
								}
							}
						}

						ItemSizeBox->AddChild(ItemVisualCanvas);
					}
					else
					{
						ItemSizeBox->AddChild(NewItemWidget);
					}

					if (UOverlaySlot* OverlaySlot = ItemContainer->AddChildToOverlay(ItemSizeBox))
					{
						OverlaySlot->SetHorizontalAlignment(HAlign_Left);
						OverlaySlot->SetVerticalAlignment(VAlign_Top);
						OverlaySlot->SetPadding(FMargin(0.0f));
					}
				}
				else
				{
					if (UOverlaySlot* OverlaySlot = ItemContainer->AddChildToOverlay(NewItemWidget))
					{
						OverlaySlot->SetHorizontalAlignment(HAlign_Left);
						OverlaySlot->SetVerticalAlignment(VAlign_Top);
						OverlaySlot->SetPadding(FMargin(0.0f));
					}
				}

				ItemWidgetInstance = NewItemWidget;
			}
		}
	}
}

void UUI_ItemSlot::UnbindItem()
{
	if (ItemWidgetInstance)
	{
		ItemWidgetInstance->RemoveFromParent();
		ItemWidgetInstance = nullptr;
	}
	if (ItemContainer)
	{
		ItemContainer->ClearChildren();
	}

	BoundItem = nullptr;
	SetOccupied(false);
}

void UUI_ItemSlot::SetItemWidgetClass(TSubclassOf<UUI_ItemWidget> InItemWidgetClass)
{
	ItemWidgetClass = InItemWidgetClass;
}

void UUI_ItemSlot::SetOwningInventory(UUI_Inventory* InOwningInventory)
{
	OwningInventory = InOwningInventory;
}

FLinearColor UUI_ItemSlot::GetRarityColor(EInventoryItemRarity Rarity) const
{
	return GetInventoryItemRaritySlotBackgroundColor(Rarity);
}