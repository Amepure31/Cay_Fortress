#include "UI/UI_ItemTooltip.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Inventory/InventoryItemRarity.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/InventoryItemDataAsset.h"

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
			if (InItemInstance->MaxDurability > 0)
			{
				DurabilityBar->SetVisibility(ESlateVisibility::Visible);
				const float Percent = static_cast<float>(InItemInstance->Durability) / static_cast<float>(InItemInstance->MaxDurability);
				DurabilityBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
			}
			else
			{
				DurabilityBar->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		if (DurabilityText)
		{
			if (InItemInstance->MaxDurability > 0)
			{
				DurabilityText->SetVisibility(ESlateVisibility::Visible);
				DurabilityText->SetText(FText::Format(
					FText::FromString(TEXT("{0}/{1}")),
					FText::AsNumber(InItemInstance->Durability),
					FText::AsNumber(InItemInstance->MaxDurability)));
			}
			else
			{
				DurabilityText->SetVisibility(ESlateVisibility::Hidden);
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

		if (TypeSpecificText)
		{
			FString DetailLine;

			if (ItemData.ItemType == EInventoryItemType::Weapon)
			{
				const FWeaponItemStats Stats = InItemInstance->GetEffectiveWeaponStats();
				DetailLine += FString::Printf(TEXT("伤害: %.0f\n"), Stats.Damage);
				DetailLine += FString::Printf(TEXT("弹夹: %d/%d\n"), InItemInstance->WeaponMagazineAmmo, Stats.MagazineCapacity);
				DetailLine += FString::Printf(TEXT("水平后坐力: %.1f  垂直后坐力: %.1f\n"), Stats.HorizontalRecoil, Stats.VerticalRecoil);
			}

			DetailLine += FString::Printf(TEXT("重量: %.2f  价值: %d"), ItemData.Weight, ItemData.Value);

			if (ItemData.ItemType == EInventoryItemType::Ammo)
			{
				const FString AmmoLine = FString::Printf(
					TEXT("子弹类型: %s"),
					*StaticEnum<EAmmoType>()->GetDisplayNameTextByValue(static_cast<int64>(ItemData.AmmoStats.AmmoType)).ToString());
				DetailLine += TEXT("\n");
				DetailLine += AmmoLine;
			}

			if (DetailLine.IsEmpty())
			{
				TypeSpecificText->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				TypeSpecificText->SetText(FText::FromString(DetailLine));
				TypeSpecificText->SetVisibility(ESlateVisibility::Visible);
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