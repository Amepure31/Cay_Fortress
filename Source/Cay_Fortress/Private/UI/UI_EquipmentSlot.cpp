#include "UI/UI_EquipmentSlot.h"
#include "UI/UI_Equipment.h"
#include "UI/UI_ItemWidget.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/DragDropOperation.h"
#include "Equipment/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemRarity.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
static FText GetSlotDisplayName(EEquipmentSlotType Slot)
{
	if (const UEnum* EnumPtr = StaticEnum<EEquipmentSlotType>())
	{
		return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(Slot));
	}
	return FText::GetEmpty();
}

static UTexture2D* LoadEngineWhiteSquareTexture()
{
	static TWeakObjectPtr<UTexture2D> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	}
	return Cached.Get();
}

static void ResolveItemIconFitBounds(UImage* ItemIcon, int32 FallbackW, int32 FallbackH, int32& OutW, int32& OutH, UScaleBox** OutScaleBox)
{
	*OutScaleBox = nullptr;
	OutW = FMath::Max(1, FallbackW);
	OutH = FMath::Max(1, FallbackH);
	if (!ItemIcon)
	{
		return;
	}

	for (UWidget* Walk = ItemIcon->GetParent(); Walk; Walk = Walk->GetParent())
	{
		if (UScaleBox* SB = Cast<UScaleBox>(Walk))
		{
			const FVector2D Sz = SB->GetCachedGeometry().GetLocalSize();
			if (Sz.X > 1.0f && Sz.Y > 1.0f)
			{
				*OutScaleBox = SB;
				OutW = FMath::Max(1, FMath::RoundToInt(Sz.X));
				OutH = FMath::Max(1, FMath::RoundToInt(Sz.Y));
				return;
			}
		}
		if (UOverlay* Ov = Cast<UOverlay>(Walk))
		{
			const FVector2D Sz = Ov->GetCachedGeometry().GetLocalSize();
			if (Sz.X > 1.0f && Sz.Y > 1.0f)
			{
				OutW = FMath::Max(1, FMath::RoundToInt(Sz.X));
				OutH = FMath::Max(1, FMath::RoundToInt(Sz.Y));
				return;
			}
		}
	}
}

static void ApplyAspectFitCenteredItemIcon(UImage* ItemIcon, UTexture2D* Tex, int32 SlotPixelW, int32 SlotPixelH)
{
	if (!ItemIcon || !Tex)
	{
		return;
	}

	UScaleBox* ScaleBoxParent = nullptr;
	int32 UseW = 0;
	int32 UseH = 0;
	ResolveItemIconFitBounds(ItemIcon, SlotPixelW, SlotPixelH, UseW, UseH, &ScaleBoxParent);

	if (ScaleBoxParent)
	{
		ScaleBoxParent->SetStretch(EStretch::ScaleToFit);
	}

	const float Pad = 8.0f;
	const float MaxW = FMath::Max(8.0f, static_cast<float>(UseW) - Pad);
	const float MaxH = FMath::Max(8.0f, static_cast<float>(UseH) - Pad);
	const float TexW = static_cast<float>(FMath::Max(1, Tex->GetSizeX()));
	const float TexH = static_cast<float>(FMath::Max(1, Tex->GetSizeY()));
	const float Scale = FMath::Min(MaxW / TexW, MaxH / TexH);
	const float DrawW = FMath::Max(4.0f, TexW * Scale);
	const float DrawH = FMath::Max(4.0f, TexH * Scale);

	FSlateBrush Brush = ItemIcon->GetBrush();
	Brush.SetResourceObject(Tex);
	Brush.ImageSize = FVector2D(DrawW, DrawH);
	ItemIcon->SetBrush(Brush);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ItemIcon->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetAutoSize(true);
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ItemIcon->Slot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Center);
		OverlaySlot->SetVerticalAlignment(VAlign_Center);
	}
	else if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(ItemIcon->Slot))
	{
		VSlot->SetHorizontalAlignment(HAlign_Center);
	}
	else if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(ItemIcon->Slot))
	{
		HSlot->SetVerticalAlignment(VAlign_Center);
	}
}
}

void UUI_EquipmentSlot::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureDesignerLayoutSyncTimer();

	if (SlotLabel)
	{
		SlotLabel->SetText(GetSlotDisplayName(SlotType));
	}

	if (HoverOverlay)
	{
		HoverOverlay->SetVisibility(ESlateVisibility::Hidden);
	}

	if (OwningPanel)
	{
		ApplySlotChromeAndSize();
	}
}

void UUI_EquipmentSlot::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DesignerLayoutSyncTimer);
	}
	Super::NativeDestruct();
}

void UUI_EquipmentSlot::EnsureDesignerLayoutSyncTimer()
{
	if (!bUseDesignerSlotSize)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->GetTimerManager().IsTimerActive(DesignerLayoutSyncTimer))
	{
		return;
	}
	World->GetTimerManager().SetTimer(
		DesignerLayoutSyncTimer,
		FTimerDelegate::CreateUObject(this, &UUI_EquipmentSlot::DesignerLayoutSyncTimerElapsed),
		0.016f,
		true);
}

void UUI_EquipmentSlot::DesignerLayoutSyncTimerElapsed()
{
	if (!bUseDesignerSlotSize || !OwningPanel)
	{
		return;
	}
	SyncDesignerIconBrushesToLayout(false);
}

void UUI_EquipmentSlot::RestoreSlotBackgroundBaseAfterDrag()
{
	if (SlotBackground)
	{
		SlotBackground->SetColorAndOpacity(CachedBackgroundBaseForDrag);
	}
}

void UUI_EquipmentSlot::SyncDesignerIconBrushesToLayout(bool bForce)
{
	if (!OwningPanel || !bUseDesignerSlotSize)
	{
		return;
	}

	float OvW = SlotSizeBox ? SlotSizeBox->GetWidthOverride() : 0.0f;
	float OvH = SlotSizeBox ? SlotSizeBox->GetHeightOverride() : 0.0f;

	FVector2D Geo(0.0f, 0.0f);
	if (SlotBackground)
	{
		Geo = SlotBackground->GetCachedGeometry().GetLocalSize();
	}
	else
	{
		Geo = GetCachedGeometry().GetLocalSize();
	}

	float W = FMath::Max(OvW, Geo.X);
	float H = FMath::Max(OvH, Geo.Y);

	if (W <= KINDA_SMALL_NUMBER || H <= KINDA_SMALL_NUMBER)
	{
		int32 WidthCells = 1;
		int32 HeightCells = 1;
		OwningPanel->GetSlotGridSize(SlotType, WidthCells, HeightCells);
		const int32 Cell = OwningPanel->GetEffectiveEquipmentCellPixelSize();
		if (W <= KINDA_SMALL_NUMBER)
		{
			W = static_cast<float>(FMath::Max(1, WidthCells) * Cell);
		}
		if (H <= KINDA_SMALL_NUMBER)
		{
			H = static_cast<float>(FMath::Max(1, HeightCells) * Cell);
		}
	}

	const int32 NewW = FMath::Max(1, FMath::RoundToInt(W));
	const int32 NewH = FMath::Max(1, FMath::RoundToInt(H));

	if (!bForce && NewW == CachedSlotPixelWidth && NewH == CachedSlotPixelHeight)
	{
		return;
	}

	CachedSlotPixelWidth = NewW;
	CachedSlotPixelHeight = NewH;

	RefreshSlotVisual();
}

void UUI_EquipmentSlot::ApplySlotChromeAndSize()
{
	if (!OwningPanel)
	{
		return;
	}

	// Capture the UMG designer brush & color before any modifications.
	// This runs during widget creation when the background is still in its
	// original designer state. Once captured, the empty-slot restore path
	// in RefreshSlotVisual will always use these stored values.
	if (SlotBackground && !bHasStoredDesignerSlotBackgroundBrush)
	{
		StoredDesignerSlotBackgroundBrush = SlotBackground->GetBrush();
		bHasStoredDesignerSlotBackgroundBrush = true;
		if (!bHasCachedManualSlotBackgroundColor)
		{
			CachedManualSlotBackgroundColor = SlotBackground->GetColorAndOpacity();
			bHasCachedManualSlotBackgroundColor = true;
		}
	}

	if (bUseDesignerSlotSize)
	{
		EnsureDesignerLayoutSyncTimer();

		SyncDesignerIconBrushesToLayout(true);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DesignerLayoutSyncTimer);
	}

	int32 WidthCells = 1;
	int32 HeightCells = 1;
	OwningPanel->GetSlotGridSize(SlotType, WidthCells, HeightCells);

	const int32 Cell = OwningPanel->GetEffectiveEquipmentCellPixelSize();
	CachedSlotPixelWidth = FMath::Max(1, WidthCells) * Cell;
	CachedSlotPixelHeight = FMath::Max(1, HeightCells) * Cell;

	if (SlotSizeBox)
	{
		SlotSizeBox->SetWidthOverride(static_cast<float>(CachedSlotPixelWidth));
		SlotSizeBox->SetHeightOverride(static_cast<float>(CachedSlotPixelHeight));
	}

	if (SlotBackground)
	{
		FSlateBrush BgBrush = SlotBackground->GetBrush();
		BgBrush.ImageSize = FVector2D(static_cast<float>(CachedSlotPixelWidth), static_cast<float>(CachedSlotPixelHeight));
		SlotBackground->SetBrush(BgBrush);
	}

	RefreshSlotVisual();
}

void UUI_EquipmentSlot::RefreshSlotVisual()
{
	UInventoryItemInstance* Equipped = nullptr;
	if (OwningPanel)
	{
		if (UEquipmentComponent* Comp = OwningPanel->GetBoundEquipment())
		{
			Equipped = Comp->GetEquippedItem(SlotType);
		}
	}

	const bool bHasEquipped = Equipped && Equipped->ItemData;

	if (SlotBackground)
	{
		const FVector2D BgGeo = SlotBackground->GetCachedGeometry().GetLocalSize();

		if (bHasEquipped)
		{
			if (UTexture2D* WhiteTex = LoadEngineWhiteSquareTexture())
			{
				SlotBackground->SetBrushFromTexture(WhiteTex);
				FSlateBrush BgBrush = SlotBackground->GetBrush();
				BgBrush.TintColor = FSlateColor(FLinearColor::White);
				if (bUseDesignerSlotSize && BgGeo.X > 1.0f && BgGeo.Y > 1.0f)
				{
					BgBrush.ImageSize = BgGeo;
				}
				else
				{
					BgBrush.ImageSize = FVector2D(
						static_cast<float>(FMath::Max(1, CachedSlotPixelWidth)),
						static_cast<float>(FMath::Max(1, CachedSlotPixelHeight)));
				}
				SlotBackground->SetBrush(BgBrush);
			}

			CachedBackgroundBaseForDrag =
				GetInventoryItemRaritySlotBackgroundColor(Equipped->ItemData->ItemData.Rarity);
			SlotBackground->SetColorAndOpacity(CachedBackgroundBaseForDrag);
		}
		else
		{
			if (bHasStoredDesignerSlotBackgroundBrush)
			{
				SlotBackground->SetBrush(StoredDesignerSlotBackgroundBrush);
			}
			const FLinearColor EmptyTint = bHasCachedManualSlotBackgroundColor
				? CachedManualSlotBackgroundColor
				: DesignerEmptyBackgroundColor;
			CachedBackgroundBaseForDrag = EmptyTint;
			SlotBackground->SetColorAndOpacity(EmptyTint);
		}
	}

	if (!ItemIcon)
	{
		return;
	}

	if (SlotHintIcon)
	{
		SlotHintIcon->SetVisibility(bHasEquipped ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}

	if (bHasEquipped && Equipped->ItemData->ItemData.Icon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Visible);

		int32 FitW = CachedSlotPixelWidth;
		int32 FitH = CachedSlotPixelHeight;
		if (FitW <= 0 || FitH <= 0)
		{
			UScaleBox* UnusedScale = nullptr;
			ResolveItemIconFitBounds(ItemIcon, 256, 256, FitW, FitH, &UnusedScale);
		}
		ApplyAspectFitCenteredItemIcon(ItemIcon, Equipped->ItemData->ItemData.Icon, FitW, FitH);
	}
	else
	{
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	}
}

FReply UUI_EquipmentSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (OwningPanel)
		{
			OwningPanel->UnequipSlot(SlotType);
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UUI_EquipmentSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetHoverActive(true);
}

void UUI_EquipmentSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetHoverActive(false);
}

bool UUI_EquipmentSlot::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UUI_ItemWidget* SourceWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	if (!SourceWidget || !SourceWidget->GetItemInstance())
	{
		return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
	}

	bool bCanEquip = false;
	if (OwningPanel)
	{
		if (UEquipmentComponent* Comp = OwningPanel->GetBoundEquipment())
		{
			UInventoryItemInstance* Item = SourceWidget->GetItemInstance();
			bCanEquip = Item->ItemData && Comp->CanEquipItemToSlot(Item->ItemData, SlotType);
		}
	}

	if (SlotBackground)
	{
		const FLinearColor Highlight = bCanEquip
			? FLinearColor(0.22f, 0.52f, 0.28f, 1.0f)
			: FLinearColor(0.52f, 0.24f, 0.22f, 1.0f);
		SlotBackground->SetColorAndOpacity(FLinearColor::LerpUsingHSV(CachedBackgroundBaseForDrag, Highlight, 0.5f));
	}

	return true;
}

void UUI_EquipmentSlot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	RestoreSlotBackgroundBaseAfterDrag();
}

bool UUI_EquipmentSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	RestoreSlotBackgroundBaseAfterDrag();

	UUI_ItemWidget* SourceWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	if (!SourceWidget || !SourceWidget->GetItemInstance())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	if (OwningPanel)
	{
		return OwningPanel->TryEquipFromDrag(SourceWidget, SlotType);
	}

	return false;
}

void UUI_EquipmentSlot::SetHoverActive(bool bActive)
{
	if (HoverOverlay)
	{
		HoverOverlay->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
