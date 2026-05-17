#include "UI/UI_ItemContextMenu.h"
#include "UI/UI_Inventory.h"
#include "Inventory/FInventoryItemInstance.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/InventoryComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"

TWeakObjectPtr<UUI_ItemContextMenu> UUI_ItemContextMenu::ActiveMenu;

void UUI_ItemContextMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (UseButton)
		UseButton->OnClicked.AddDynamic(this, &UUI_ItemContextMenu::OnUseClicked);
	if (DiscardButton)
		DiscardButton->OnClicked.AddDynamic(this, &UUI_ItemContextMenu::OnDiscardClicked);
}

void UUI_ItemContextMenu::Show(UUI_Inventory* InInventory, UInventoryItemInstance* InItem, FVector2D ScreenPosition)
{
	if (!InInventory || !InItem || !InItem->ItemData) return;

	// Close any previously open menu
	if (ActiveMenu.IsValid() && ActiveMenu.Get() != this)
		ActiveMenu->CloseMenu();
	ActiveMenu = this;

	OwningInventory = InInventory;
	TargetItem = InItem;

	const EInventoryItemType ItemType = InItem->ItemData->ItemData.ItemType;
	const bool bCanUse = ItemType == EInventoryItemType::Weapon || ItemType == EInventoryItemType::Armor || ItemType == EInventoryItemType::Food || ItemType == EInventoryItemType::Medical;
	const bool bIsQuest = ItemType == EInventoryItemType::Quest;

	// Use button — only for weapon, armor, food
	if (UseButton)
	{
		UseButton->SetVisibility(bCanUse ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (UseButtonText && bCanUse)
		{
			if (ItemType == EInventoryItemType::Food)
				UseButtonText->SetText(FText::FromString(TEXT("食用")));
			else if (ItemType == EInventoryItemType::Medical)
				UseButtonText->SetText(FText::FromString(TEXT("治疗")));
			else
				UseButtonText->SetText(FText::FromString(TEXT("使用")));
		}
	}

	// Discard button — all items except quest
	if (DiscardButton)
	{
		DiscardButton->SetVisibility(bIsQuest ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	// Don't show menu if nothing to show (quest item)
	if (bIsQuest || !bCanUse)
	{
		// at least one button must be visible
		const bool bUseVisible = UseButton && UseButton->GetVisibility() != ESlateVisibility::Collapsed;
		const bool bDiscardVisible = DiscardButton && DiscardButton->GetVisibility() != ESlateVisibility::Collapsed;
		if (!bUseVisible && !bDiscardVisible) return;
	}

	AddToViewport(100);
	SetPositionInViewport(ScreenPosition, true);
	ForceLayoutPrepass();
}

void UUI_ItemContextMenu::OnUseClicked()
{
	if (!OwningInventory.IsValid() || !TargetItem.IsValid()) { CloseMenu(); return; }

	UInventoryItemInstance* Item = TargetItem.Get();
	UUI_Inventory* Inv = OwningInventory.Get();
	const EInventoryItemType ItemType = Item->ItemData->ItemData.ItemType;

	if (ItemType == EInventoryItemType::Weapon || ItemType == EInventoryItemType::Armor)
	{
		Inv->RequestEquipItem(Item);
	}
	else if (ItemType == EInventoryItemType::Food)
	{
		Inv->RequestConsumeFood(Item);
	}
	else if (ItemType == EInventoryItemType::Medical)
	{
		Inv->RequestConsumeMedical(Item);
	}

	CloseMenu();
}

void UUI_ItemContextMenu::OnDiscardClicked()
{
	if (!OwningInventory.IsValid() || !TargetItem.IsValid()) { CloseMenu(); return; }
	OwningInventory->RequestDiscardItem(TargetItem.Get());
	CloseMenu();
}

void UUI_ItemContextMenu::CloseMenu()
{
	if (ActiveMenu.Get() == this)
		ActiveMenu.Reset();
	if (IsInViewport()) RemoveFromParent();
}

void UUI_ItemContextMenu::CloseActiveMenu()
{
	if (ActiveMenu.IsValid())
		ActiveMenu->CloseMenu();
}

void UUI_ItemContextMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Skip first few ticks so the right-click that opened the menu doesn't immediately close it
	static constexpr float DismissDelay = 0.15f;
	Elapsed += InDeltaTime;
	if (Elapsed < DismissDelay) return;

	// Close menu when clicking anywhere outside it
	if (FSlateApplication::Get().IsInitialized() && FSlateApplication::Get().GetPressedMouseButtons().Num() > 0)
	{
		const FVector2f CursorPos = FSlateApplication::Get().GetCursorPos();
		const FVector2f AbsPos = MyGeometry.GetAbsolutePosition();
		const FVector2f Size = MyGeometry.GetAbsoluteSize();
		if (CursorPos.X < AbsPos.X || CursorPos.Y < AbsPos.Y || CursorPos.X > AbsPos.X + Size.X || CursorPos.Y > AbsPos.Y + Size.Y)
		{
			CloseMenu();
		}
	}
}
