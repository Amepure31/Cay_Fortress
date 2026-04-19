// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_ItemWidget.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
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

namespace
{
static constexpr float ItemWidgetMinCellPixelSize = 64.0f;

static UWidget* CreateItemWidgetConstrainedDragVisual(
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

	float CellPixelSize = ItemWidgetMinCellPixelSize;
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

static FFItemShapeMask MakeFullMask(int32 Width, int32 Height)
{
	FFItemShapeMask Result;
	Result.Width = FMath::Max(1, Width);
	Result.Height = FMath::Max(1, Height);
	Result.ShapeMaskData.Init(1, Result.Width * Result.Height);
	return Result;
}

static FFItemShapeMask NormalizeMask(const FFItemShapeMask& InMask, int32 Width, int32 Height)
{
	const int32 SafeWidth = FMath::Max(1, Width);
	const int32 SafeHeight = FMath::Max(1, Height);
	if (InMask.Width <= 0 || InMask.Height <= 0 || InMask.ShapeMaskData.Num() < InMask.Width * InMask.Height)
	{
		return MakeFullMask(SafeWidth, SafeHeight);
	}

	FFItemShapeMask Result;
	Result.Width = SafeWidth;
	Result.Height = SafeHeight;
	Result.ShapeMaskData.Init(0, SafeWidth * SafeHeight);
	for (int32 Y = 0; Y < SafeHeight; ++Y)
	{
		for (int32 X = 0; X < SafeWidth; ++X)
		{
			Result.ShapeMaskData[Y * SafeWidth + X] = InMask.IsOccupied(X, Y) ? 1 : 0;
		}
	}
	return Result;
}

static FFItemShapeMask RotateMaskClockwise(const FFItemShapeMask& InMask)
{
	const int32 OldWidth = FMath::Max(1, InMask.Width);
	const int32 OldHeight = FMath::Max(1, InMask.Height);
	const int32 NewWidth = OldHeight;
	const int32 NewHeight = OldWidth;

	FFItemShapeMask RotatedMask;
	RotatedMask.Width = NewWidth;
	RotatedMask.Height = NewHeight;
	RotatedMask.ShapeMaskData.Init(0, NewWidth * NewHeight);

	for (int32 Y = 0; Y < OldHeight; ++Y)
	{
		for (int32 X = 0; X < OldWidth; ++X)
		{
			if (!InMask.IsOccupied(X, Y))
			{
				continue;
			}

			const int32 RotatedX = OldHeight - 1 - Y;
			const int32 RotatedY = X;
			RotatedMask.ShapeMaskData[RotatedY * NewWidth + RotatedX] = 1;
		}
	}

	return RotatedMask;
}

static bool IsFullRectangularMask(const FFItemShapeMask& InMask)
{
	const int32 Width = FMath::Max(1, InMask.Width);
	const int32 Height = FMath::Max(1, InMask.Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (!InMask.IsOccupied(X, Y))
			{
				return false;
			}
		}
	}
	return true;
}

static void ForceStackTextBottomRight(UTextBlock* StackText, const FVector2D* ItemPixelSize)
{
	if (!StackText || !StackText->Slot)
	{
		return;
	}

	StackText->SetJustification(ETextJustify::Right);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(StackText->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		CanvasSlot->SetZOrder(100);
		StackText->SetRenderTransformPivot(FVector2D(1.0f, 1.0f));
		StackText->SetRenderTranslation(FVector2D::ZeroVector);
		if (ItemPixelSize)
		{
			// Pin text bottom-right using slot position; this is resilient to container transforms.
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			CanvasSlot->SetPosition(FVector2D(ItemPixelSize->X - 4.0f, ItemPixelSize->Y - 4.0f));
			CanvasSlot->SetSize(FVector2D::ZeroVector);
		}
		else
		{
			CanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
			CanvasSlot->SetOffsets(FMargin(-4.0f, -4.0f, 0.0f, 0.0f));
		}
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(StackText->Slot))
	{
		StackText->SetRenderTranslation(FVector2D::ZeroVector);
		OverlaySlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 4.0f));
		OverlaySlot->SetHorizontalAlignment(HAlign_Right);
		OverlaySlot->SetVerticalAlignment(VAlign_Bottom);
	}
	else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(StackText->Slot))
	{
		StackText->SetRenderTranslation(FVector2D::ZeroVector);
		BorderSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 4.0f));
		BorderSlot->SetHorizontalAlignment(HAlign_Right);
		BorderSlot->SetVerticalAlignment(VAlign_Bottom);
	}
}

static void NormalizeItemIconSlot(UImage* ItemIcon)
{
	if (!ItemIcon || !ItemIcon->Slot)
	{
		return;
	}

	// Root canvas slots can keep designer-time offsets that skew runtime layout.
	// Normalize the slot so icon starts at (0,0) and scales from desired size.
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ItemIcon->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ItemIcon->Slot))
	{
		OverlaySlot->SetPadding(FMargin(0.0f));
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(ItemIcon->Slot))
	{
		BorderSlot->SetPadding(FMargin(0.0f));
		BorderSlot->SetHorizontalAlignment(HAlign_Fill);
		BorderSlot->SetVerticalAlignment(VAlign_Fill);
	}
}
}

UInventoryItemInstance* UUI_ItemWidget::GetItemInstance() const
{
	return BoundItem;
}

void UUI_ItemWidget::SetItemInstance(UInventoryItemInstance* InItemInstance)
{
	BoundItem = InItemInstance;
	bDragSessionActive = false;
	DragRotationQuarterTurns = (BoundItem ? ((BoundItem->RotationQuarterTurns % 4) + 4) % 4 : 0);
	ActiveDragVisual = nullptr;
	ActiveDragVisualHost = nullptr;
	ActiveDragOperation = nullptr;
	if (!InItemInstance || !InItemInstance->ItemData)
	{
		return;
	}

	if (ItemIcon)
	{
		UTexture2D* IconTexture = InItemInstance->ItemData->ItemData.Icon;
		ItemIcon->SetBrushFromTexture(IconTexture);
		NormalizeItemIconSlot(ItemIcon);
	}
	ApplyVisualDimensions(InItemInstance->Width, InItemInstance->Height);
	UpdateStackSizeDisplay();
}

void UUI_ItemWidget::BeginDragSession()
{
	if (!BoundItem)
	{
		return;
	}

	bDragSessionActive = true;
	SourceVisualRootBeforeDrag = nullptr;
	DragRotationQuarterTurns = ((BoundItem->RotationQuarterTurns % 4) + 4) % 4;
	DragFootprintWidth = FMath::Max(1, BoundItem->Width);
	DragFootprintHeight = FMath::Max(1, BoundItem->Height);
	DragFootprintMask = NormalizeMask(BoundItem->ShapeMask, DragFootprintWidth, DragFootprintHeight);
	DragBaseWidth = DragFootprintWidth;
	DragBaseHeight = DragFootprintHeight;
	DragBaseMask = DragFootprintMask;
	DragBaseRotationQuarterTurns = DragRotationQuarterTurns;
}

void UUI_ItemWidget::EndDragSession()
{
	bDragSessionActive = false;
	DragRotationQuarterTurns = (BoundItem ? ((BoundItem->RotationQuarterTurns % 4) + 4) % 4 : 0);
	ActiveDragVisual = nullptr;
	ActiveDragVisualHost = nullptr;
	ActiveDragOperation = nullptr;
	RestoreSourceVisualRootAfterDrag();

	if (BoundItem)
	{
		ApplyVisualDimensions(BoundItem->Width, BoundItem->Height);
	}
}

void UUI_ItemWidget::RotateDragFootprintClockwise()
{
	if (!bDragSessionActive || !BoundItem)
	{
		return;
	}

	// Single-cell items are explicitly non-rotatable.
	if (DragBaseWidth == 1 && DragBaseHeight == 1)
	{
		return;
	}

	// Full rectangular items only toggle between base orientation and +90 degree orientation.
	if (IsFullRectangularMask(DragBaseMask))
	{
		const int32 PreviousWidth = DragFootprintWidth;
		const int32 PreviousHeight = DragFootprintHeight;
		const int32 PreviousGrabX = DragGrabCellX;
		const int32 PreviousGrabY = DragGrabCellY;

		// Toggle strictly between 0° and 90° to avoid drifting into 180°/270°.
		const bool bCurrentlyHorizontal = ((DragRotationQuarterTurns % 2) == 0);
		DragRotationQuarterTurns = bCurrentlyHorizontal ? 1 : 0;

		DragFootprintWidth = PreviousHeight;
		DragFootprintHeight = PreviousWidth;
		DragFootprintMask = MakeFullMask(DragFootprintWidth, DragFootprintHeight);

		// Keep cursor attached to same logical cell through the 90-degree toggle.
		DragGrabCellX = PreviousHeight - 1 - PreviousGrabY;
		DragGrabCellY = PreviousGrabX;

		DragGrabCellX = FMath::Clamp(DragGrabCellX, 0, FMath::Max(0, DragFootprintWidth - 1));
		DragGrabCellY = FMath::Clamp(DragGrabCellY, 0, FMath::Max(0, DragFootprintHeight - 1));

		UpdateActiveDragVisualHostSize();
		UpdateDragOperationAnchor();
		ApplyVisualDimensions(DragFootprintWidth, DragFootprintHeight);

		if (ActiveDragVisual && ActiveDragVisual != this)
		{
			ActiveDragVisual->SetDragFootprintInternal(DragFootprintWidth, DragFootprintHeight, DragFootprintMask, DragRotationQuarterTurns);
		}
		return;
	}

	const int32 PreviousHeight = DragFootprintHeight;
	const int32 PreviousGrabX = DragGrabCellX;
	const int32 PreviousGrabY = DragGrabCellY;

	DragFootprintMask = RotateMaskClockwise(DragFootprintMask);
	DragFootprintWidth = DragFootprintMask.Width;
	DragFootprintHeight = DragFootprintMask.Height;
	DragRotationQuarterTurns = (DragRotationQuarterTurns + 1) % 4;

	// Keep the cursor attached to the same occupied cell after rotation.
	DragGrabCellX = PreviousHeight - 1 - PreviousGrabY;
	DragGrabCellY = PreviousGrabX;
	DragGrabCellX = FMath::Clamp(DragGrabCellX, 0, FMath::Max(0, DragFootprintWidth - 1));
	DragGrabCellY = FMath::Clamp(DragGrabCellY, 0, FMath::Max(0, DragFootprintHeight - 1));

	// Refresh drag host and anchor first, so Slate doesn't render one frame with stale geometry.
	UpdateActiveDragVisualHostSize();
	UpdateDragOperationAnchor();

	ApplyVisualDimensions(DragFootprintWidth, DragFootprintHeight);

	if (ActiveDragVisual && ActiveDragVisual != this)
	{
		ActiveDragVisual->SetDragFootprintInternal(DragFootprintWidth, DragFootprintHeight, DragFootprintMask, DragRotationQuarterTurns);
	}
}

int32 UUI_ItemWidget::GetCurrentDragWidth() const
{
	return bDragSessionActive ? DragFootprintWidth : (BoundItem ? FMath::Max(1, BoundItem->Width) : 1);
}

int32 UUI_ItemWidget::GetCurrentDragHeight() const
{
	return bDragSessionActive ? DragFootprintHeight : (BoundItem ? FMath::Max(1, BoundItem->Height) : 1);
}

const FFItemShapeMask& UUI_ItemWidget::GetCurrentDragShapeMask() const
{
	if (bDragSessionActive || !BoundItem)
	{
		return DragFootprintMask;
	}
	return BoundItem->ShapeMask;
}

void UUI_ItemWidget::SetActiveDragVisual(UUI_ItemWidget* InDragVisual)
{
	ActiveDragVisual = InDragVisual;
	if (ActiveDragVisual && bDragSessionActive)
	{
		ActiveDragVisual->SetDragFootprintInternal(DragFootprintWidth, DragFootprintHeight, DragFootprintMask, DragRotationQuarterTurns);
	}
}

void UUI_ItemWidget::SetActiveDragVisualHost(USizeBox* InDragVisualHost)
{
	ActiveDragVisualHost = InDragVisualHost;
	UpdateActiveDragVisualHostSize();
}

void UUI_ItemWidget::SetActiveDragOperation(UDragDropOperation* InDragOperation)
{
	ActiveDragOperation = InDragOperation;
	UpdateDragOperationAnchor();
}

void UUI_ItemWidget::UpdateActiveDragVisualHostSize()
{
	if (!ActiveDragVisualHost)
	{
		return;
	}

	float CellPixelSize = ItemWidgetMinCellPixelSize;
	if (const UUI_Inventory* InventoryWidget = OwningInventory.Get())
	{
		CellPixelSize = static_cast<float>(InventoryWidget->GetEffectiveSlotSize());
	}

	ActiveDragVisualHost->SetWidthOverride(static_cast<float>(GetCurrentDragWidth()) * CellPixelSize);
	ActiveDragVisualHost->SetHeightOverride(static_cast<float>(GetCurrentDragHeight()) * CellPixelSize);
}

void UUI_ItemWidget::UpdateDragOperationAnchor()
{
	if (!ActiveDragOperation)
	{
		return;
	}

	const int32 Width = FMath::Max(1, GetCurrentDragWidth());
	const int32 Height = FMath::Max(1, GetCurrentDragHeight());
	const int32 ClampedGrabX = FMath::Clamp(DragGrabCellX, 0, Width - 1);
	const int32 ClampedGrabY = FMath::Clamp(DragGrabCellY, 0, Height - 1);

	const float RelativeX = (static_cast<float>(ClampedGrabX) + 0.5f) / static_cast<float>(Width);
	const float RelativeY = (static_cast<float>(ClampedGrabY) + 0.5f) / static_cast<float>(Height);

	// Keep cursor anchored to the same logical grabbed grid cell after rotation.
	ActiveDragOperation->Pivot = EDragPivot::TopLeft;
	ActiveDragOperation->Offset = FVector2D(-RelativeX, -RelativeY);
}

void UUI_ItemWidget::HideSourceVisualRootForDrag()
{
	if (UPanelWidget* ParentPanel = GetParent())
	{
		SourceVisualRootBeforeDrag = ParentPanel;
		SourceVisualRootBeforeDrag->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UUI_ItemWidget::RestoreSourceVisualRootAfterDrag()
{
	if (SourceVisualRootBeforeDrag)
	{
		SourceVisualRootBeforeDrag->SetVisibility(ESlateVisibility::Visible);
		SourceVisualRootBeforeDrag = nullptr;
	}
}

void UUI_ItemWidget::ApplyVisualDimensions(int32 InWidth, int32 InHeight)
{
	if (!ItemIcon)
	{
		ForceStackTextBottomRight(StackSizeText, nullptr);
		return;
	}

	const float FootprintWidthCells = static_cast<float>(FMath::Max(1, InWidth));
	const float FootprintHeightCells = static_cast<float>(FMath::Max(1, InHeight));
	const bool bOddQuarterTurn = (DragRotationQuarterTurns & 1) != 0;
	const float RenderWidthCells = bOddQuarterTurn ? FootprintHeightCells : FootprintWidthCells;
	const float RenderHeightCells = bOddQuarterTurn ? FootprintWidthCells : FootprintHeightCells;

	float BaseCellPixelSize = ItemWidgetMinCellPixelSize;
	if (const UUI_Inventory* InventoryWidget = OwningInventory.Get())
	{
		BaseCellPixelSize = static_cast<float>(InventoryWidget->GetEffectiveSlotSize());
	}

	const FVector2D FootprintPixelSize = FVector2D(FootprintWidthCells, FootprintHeightCells) * BaseCellPixelSize;
	const FVector2D RenderPixelSize = FVector2D(RenderWidthCells, RenderHeightCells) * BaseCellPixelSize;
	ItemIcon->SetDesiredSizeOverride(RenderPixelSize);
	ForceStackTextBottomRight(StackSizeText, &FootprintPixelSize);

	FSlateBrush IconBrush = ItemIcon->GetBrush();
	IconBrush.SetImageSize(RenderPixelSize);
	ItemIcon->SetBrush(IconBrush);
	ItemIcon->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	ItemIcon->SetRenderTransformAngle(static_cast<float>(DragRotationQuarterTurns * 90));

	const FVector2D CenteringOffset = (FootprintPixelSize - RenderPixelSize) * 0.5f;
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ItemIcon->Slot))
	{
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(RenderPixelSize);
		CanvasSlot->SetPosition(CenteringOffset);
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ItemIcon->Slot))
	{
		OverlaySlot->SetPadding(FMargin(0.0f));
		OverlaySlot->SetHorizontalAlignment(HAlign_Center);
		OverlaySlot->SetVerticalAlignment(VAlign_Center);
	}
	else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(ItemIcon->Slot))
	{
		BorderSlot->SetPadding(FMargin(0.0f));
		BorderSlot->SetHorizontalAlignment(HAlign_Center);
		BorderSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UUI_ItemWidget::SetDragFootprintInternal(int32 InWidth, int32 InHeight, const FFItemShapeMask& InMask, int32 InRotationQuarterTurns)
{
	bDragSessionActive = true;
	DragFootprintWidth = FMath::Max(1, InWidth);
	DragFootprintHeight = FMath::Max(1, InHeight);
	DragFootprintMask = NormalizeMask(InMask, DragFootprintWidth, DragFootprintHeight);
	DragRotationQuarterTurns = ((InRotationQuarterTurns % 4) + 4) % 4;
	ApplyVisualDimensions(DragFootprintWidth, DragFootprintHeight);
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
	ForceStackTextBottomRight(StackSizeText, nullptr);
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
	BeginDragSession();

	UUI_ItemWidget* DragVisual = CreateWidget<UUI_ItemWidget>(GetWorld(), GetClass());
	UWidget* DragVisualHost = DragVisual;
	if (DragVisual)
	{
		DragVisual->SetOwningInventory(OwningInventory.Get());
		DragVisual->SetItemInstance(BoundItem);
		DragVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetActiveDragVisual(DragVisual);
		DragVisualHost = CreateItemWidgetConstrainedDragVisual(
			this,
			DragVisual,
			OwningInventory.Get(),
			GetCurrentDragWidth(),
			GetCurrentDragHeight()
		);
		SetActiveDragVisualHost(Cast<USizeBox>(DragVisualHost));
	}

	DragOperation->Payload = this;
	DragOperation->DefaultDragVisual = DragVisualHost;
	SetActiveDragOperation(DragOperation);
	OutOperation = DragOperation;
	HideSourceVisualRootForDrag();
	SetVisibility(ESlateVisibility::Hidden);
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
	else
	{
		EndDragSession();
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
	// Tooltip should appear across the full draggable footprint area.
	if (bResolved)
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

