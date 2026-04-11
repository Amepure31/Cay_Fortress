#include "UI/UI_ItemTooltip.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Inventory/InventoryItemRarity.h"

void UUI_ItemTooltip::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Hidden);
}

void UUI_ItemTooltip::SetItem(UInventoryItemInstance* InItemInstance)
{
	if (InItemInstance && InItemInstance->ItemData)
	{
		const FInventoryItemData& ItemData = InItemInstance->ItemData->ItemData;

		if (ItemName)
		{
			ItemName->SetText(ItemData.ItemName);
			ItemName->SetColorAndOpacity(GetRarityColor(ItemData.Rarity));
			ItemName->SetVisibility(ESlateVisibility::Visible);
		}

		if (ItemDescription)
		{
			ItemDescription->SetText(ItemData.Description);
			ItemDescription->SetVisibility(ESlateVisibility::Visible);
		}

		if (DurabilityBar)
		{
			if (InItemInstance->MaxDurability > 1)
			{
				DurabilityBar->SetVisibility(ESlateVisibility::Visible);
				DurabilityBar->SetPercent(static_cast<float>(InItemInstance->Durability) / InItemInstance->MaxDurability);
			}
			else
			{
				DurabilityBar->SetVisibility(ESlateVisibility::Hidden);
			}
		}

		if (StackSizeText)
		{
			if (ItemData.bCanStack)
			{
				StackSizeText->SetVisibility(ESlateVisibility::Visible);
				StackSizeText->SetText(FText::Format(FText::FromString(TEXT("{0}/{1}")), InItemInstance->StackSize, ItemData.MaxStackSize));
			}
			else
			{
				StackSizeText->SetVisibility(ESlateVisibility::Hidden);
			}
		}

		SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		HideTooltip();
	}
}

void UUI_ItemTooltip::HideTooltip()
{
	SetVisibility(ESlateVisibility::Hidden);
}

FLinearColor UUI_ItemTooltip::GetRarityColor(EInventoryItemRarity Rarity) const
{
	switch (Rarity)
	{
	case EInventoryItemRarity::Normal:
		return FLinearColor(0.8f, 0.8f, 0.8f);
	case EInventoryItemRarity::Uncommon:
		return FLinearColor(0.125f, 0.875f, 0.0f);
	case EInventoryItemRarity::Rare:
		return FLinearColor(0.0f, 0.4f, 1.0f);
	case EInventoryItemRarity::Epic:
		return FLinearColor(0.625f, 0.25f, 0.9375f);
	case EInventoryItemRarity::Legendary:
		return FLinearColor(1.0f, 0.625f, 0.0f);
	default:
		return FLinearColor(0.8f, 0.8f, 0.8f);
	}
}