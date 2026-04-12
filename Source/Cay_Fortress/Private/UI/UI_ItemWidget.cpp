// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_ItemWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Inventory/InventoryItemRarity.h"
#include "Inventory/FInventoryItemInstance.h"
#include "UI/UI_Inventory.h"

UInventoryItemInstance* UUI_ItemWidget::GetItemInstance() const
{
	return BoundItem;
}

void UUI_ItemWidget::SetItemInstance(UInventoryItemInstance* InItemInstance)
{
	BoundItem = InItemInstance;
	if (InItemInstance && InItemInstance->ItemData)
	{
		if (ItemIcon)
		{
			ItemIcon->SetBrushFromTexture(InItemInstance->ItemData->ItemData.Icon);
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
		if (UUI_Inventory* InventoryWidget = Cast<UUI_Inventory>(GetRootWidget()))
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
	BoundItem = nullptr;
}

void UUI_ItemWidget::NativeDestruct()
{
	BoundItem = nullptr;
	Super::NativeDestruct();
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

