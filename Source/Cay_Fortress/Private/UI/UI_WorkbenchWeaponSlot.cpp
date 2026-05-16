#include "UI/UI_WorkbenchWeaponSlot.h"
#include "UI/UI_WeaponWorkbench.h"
#include "UI/UI_ItemWidget.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/InventoryComponent.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogWorkbenchSlot, Log, All);

void UUI_WorkbenchWeaponSlot::RefreshWithWeapon(UInventoryItemInstance* Weapon)
{
	UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] RefreshWithWeapon called — Weapon=%s, ItemIcon=%s"),
		Weapon ? *Weapon->GetName() : TEXT("null"),
		ItemIcon ? TEXT("valid") : TEXT("NULL"));

	CurrentWeapon = Weapon;
	if (!ItemIcon)
	{
		UE_LOG(LogWorkbenchSlot, Warning, TEXT("[WorkbenchSlot] RefreshWithWeapon — ItemIcon is NULL, aborting"));
		return;
	}

	if (Weapon && Weapon->ItemData && Weapon->ItemData->ItemData.Icon)
	{
		// Compute mask bounding rect
		int32 OccW = Weapon->ItemData->ItemData.Width, OccH = Weapon->ItemData->ItemData.Height;
		const int32 MaskLen = Weapon->ItemData->ItemData.ShapeMaskData.Num();
		if (MaskLen >= OccW * OccH)
		{
			int32 MinX = OccW, MaxX = 0, MinY = OccH, MaxY = 0;
			for (int32 Y = 0; Y < OccH; ++Y)
				for (int32 X = 0; X < OccW; ++X)
					if (Weapon->ItemData->ItemData.ShapeMaskData[Y * OccW + X] != 0)
						{ MinX = FMath::Min(MinX, X); MaxX = FMath::Max(MaxX, X); MinY = FMath::Min(MinY, Y); MaxY = FMath::Max(MaxY, Y); }
			if (MaxX >= MinX && MaxY >= MinY) { OccW = MaxX - MinX + 1; OccH = MaxY - MinY + 1; }
		}
		if (OccW <= 0) OccW = 1; if (OccH <= 0) OccH = 1;

		// Fixed height 128, width proportional, max 384
		const float FixedH = 128.0f;
		const float Scale = FixedH / OccH;
		const float ScaledW = FMath::Min(OccW * Scale, 384.0f);

		FSlateBrush Brush = ItemIcon->GetBrush();
		Brush.SetResourceObject(Weapon->ItemData->ItemData.Icon);
		Brush.ImageSize = FVector2D(ScaledW, FixedH);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		ItemIcon->SetBrush(Brush);
		ItemIcon->SetVisibility(ESlateVisibility::Visible);

		UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Icon set — ImageSize=(%.0f, %.0f), Visibility=Visible, IconName=%s"),
			ScaledW, FixedH, *Weapon->ItemData->ItemData.Icon->GetName());

		if (SlotHintIcon) SlotHintIcon->SetVisibility(ESlateVisibility::Collapsed);

		if (SlotBackground && Weapon->ItemData)
		{
			const FLinearColor RarityColor = GetInventoryItemRaritySlotBackgroundColor(Weapon->ItemData->ItemData.Rarity);
			SlotBackground->SetColorAndOpacity(RarityColor);
			UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Background set to rarity color (R=%.2f G=%.2f B=%.2f)"),
				RarityColor.R, RarityColor.G, RarityColor.B);
		}

		if (SlotLabel)
		{
			const FText Name = Weapon->ItemData->ItemData.ItemName.IsEmpty()
				? FText::FromString(Weapon->ItemData->GetName())
				: Weapon->ItemData->ItemData.ItemName;
			SlotLabel->SetText(Name);
			SlotLabel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Label set to '%s'"), *Name.ToString());
		}
	}
	else
	{
		UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Hiding ItemIcon — Weapon=%s, HasIcon=%s"),
			Weapon ? TEXT("valid") : TEXT("null"),
			(Weapon && Weapon->ItemData && Weapon->ItemData->ItemData.Icon) ? TEXT("yes") : TEXT("no"));

		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		if (SlotHintIcon) SlotHintIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (SlotBackground) SlotBackground->SetColorAndOpacity(FLinearColor(0.52f, 0.46f, 0.42f, 1.0f));
		if (SlotLabel) SlotLabel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UUI_WorkbenchWeaponSlot::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UUI_ItemWidget* SourceWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	if (!SourceWidget || !SourceWidget->GetItemInstance())
	{
		UE_LOG(LogWorkbenchSlot, Verbose, TEXT("[WorkbenchSlot] DragOver — no valid ItemWidget payload"));
		return false;
	}

	UInventoryItemInstance* Item = SourceWidget->GetItemInstance();
	const bool bIsWeapon = Item && Item->ItemData && Item->ItemData->ItemData.ItemType == EInventoryItemType::Weapon;

	UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] DragOver — Item=%s, ItemType=%d, IsWeapon=%s"),
		Item ? *Item->GetName() : TEXT("null"),
		(Item && Item->ItemData) ? static_cast<int32>(Item->ItemData->ItemData.ItemType) : -1,
		bIsWeapon ? TEXT("ACCEPT") : TEXT("REJECT"));

	return bIsWeapon;
}

bool UUI_WorkbenchWeaponSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] NativeOnDrop — received drop"));

	UUI_ItemWidget* SourceWidget = InOperation ? Cast<UUI_ItemWidget>(InOperation->Payload) : nullptr;
	if (!SourceWidget || !SourceWidget->GetItemInstance())
	{
		UE_LOG(LogWorkbenchSlot, Warning, TEXT("[WorkbenchSlot] Drop — no valid ItemWidget payload"));
		return false;
	}

	UInventoryItemInstance* Item = SourceWidget->GetItemInstance();
	if (!Item || !Item->ItemData || Item->ItemData->ItemData.ItemType != EInventoryItemType::Weapon)
	{
		UE_LOG(LogWorkbenchSlot, Warning, TEXT("[WorkbenchSlot] Drop — item not a weapon, rejecting"));
		return false;
	}

	UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Drop ACCEPTED — Weapon=%s"), *Item->GetName());

	// Remove weapon from player inventory so it's "in" the workbench slot
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (UInventoryComponent* PlayerInv = Pawn->FindComponentByClass<UInventoryComponent>())
			{
				if (PlayerInv->Items.Contains(Item))
				{
					PlayerInv->DetachItemInstance(Item);
					UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Weapon detached from player inventory"));
				}
			}
		}
	}

	RefreshWithWeapon(Item);

	if (UUI_WeaponWorkbench* WB = ParentWorkbench.Get())
	{
		UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Notifying ParentWorkbench->OnWeaponSlotChanged"));
		WB->OnWeaponSlotChanged(Item);
	}
	else
	{
		UE_LOG(LogWorkbenchSlot, Warning, TEXT("[WorkbenchSlot] ParentWorkbench is null — OnWeaponSlotChanged NOT called"));
	}

	return true;
}

FReply UUI_WorkbenchWeaponSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Right-click — returning weapon '%s' to backpack"),
			CurrentWeapon ? *CurrentWeapon->GetName() : TEXT("null"));
		ClearWeapon();
		return FReply::Handled();
	}

	// Left-click: consumed but no drag-out
	return FReply::Handled();
}

void UUI_WorkbenchWeaponSlot::ClearWeapon()
{
	UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] ClearWeapon — clearing CurrentWeapon=%s"),
		CurrentWeapon ? *CurrentWeapon->GetName() : TEXT("null"));

	// Ensure weapon is returned to player's inventory before unbinding
	if (CurrentWeapon && CurrentWeapon->ItemData)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (UInventoryComponent* PlayerInv = Pawn->FindComponentByClass<UInventoryComponent>())
				{
					if (!PlayerInv->Items.Contains(CurrentWeapon))
					{
						if (PlayerInv->AttachItemInstance(CurrentWeapon))
						{
							UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Weapon returned to player inventory via AttachItemInstance"));
						}
						else
						{
							UE_LOG(LogWorkbenchSlot, Warning, TEXT("[WorkbenchSlot] Failed to return weapon to inventory (no space?)"));
						}
					}
					else
					{
						UE_LOG(LogWorkbenchSlot, Log, TEXT("[WorkbenchSlot] Weapon already in player inventory, no action needed"));
					}
				}
			}
		}
	}

	CurrentWeapon = nullptr;
	if (UUI_WeaponWorkbench* WB = ParentWorkbench.Get())
		WB->OnWeaponSlotChanged(nullptr);
	RefreshWithWeapon(nullptr);
}

void UUI_WorkbenchWeaponSlot::RefreshSlotVisual()
{
	UE_LOG(LogWorkbenchSlot, Verbose, TEXT("[WorkbenchSlot] RefreshSlotVisual — CurrentWeapon=%s"),
		CurrentWeapon ? *CurrentWeapon->GetName() : TEXT("null"));
	RefreshWithWeapon(CurrentWeapon);
}
