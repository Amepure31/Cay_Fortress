// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "InventoryItemRarity.generated.h"

/**
 * 物品稀有度枚举
 * 0-5级，从低到高
 */
UENUM()
enum class EInventoryItemRarity : uint8
{
	/** 普通 - 0级 */
	Normal    = 0 UMETA(DisplayName = "普通"),
	
	/** 稀有 - 1级 */
	Uncommon  = 1 UMETA(DisplayName = "稀有"),
	
	/** 罕见 - 2级 */
	Rare      = 2 UMETA(DisplayName = "罕见"),
	
	/** 史诗 - 3级 */
	Epic      = 3 UMETA(DisplayName = "史诗"),
	
	/** 传奇 - 4级 */
	Legendary = 4 UMETA(DisplayName = "传奇"),
	
	/** 神话 - 5级 */
	Mythic    = 5 UMETA(DisplayName = "神话"),

	/** 枚举上限（隐藏） */
	MAX       = 6 UMETA(Hidden)
};

/**
 * 物品稀有度枚举类
 * 用于UE反射系统
 */
UCLASS()
class CAY_FORTRESS_API UInventoryItemRarity : public UEnum
{
	GENERATED_BODY()
	
};

/** 背包格子、装备槽等「物品底」用色，与 UUI_ItemSlot::GetRarityColor 一致。 */
inline FLinearColor GetInventoryItemRaritySlotBackgroundColor(EInventoryItemRarity Rarity)
{
	switch (Rarity)
	{
	case EInventoryItemRarity::Normal:
		return FLinearColor(0.32f, 0.34f, 0.36f, 1.0f);
	case EInventoryItemRarity::Uncommon:
		return FLinearColor(0.30f, 0.38f, 0.24f, 1.0f);
	case EInventoryItemRarity::Rare:
		return FLinearColor(0.20f, 0.33f, 0.50f, 1.0f);
	case EInventoryItemRarity::Epic:
		return FLinearColor(0.42f, 0.26f, 0.49f, 1.0f);
	case EInventoryItemRarity::Legendary:
		return FLinearColor(0.62f, 0.43f, 0.18f, 1.0f);
	case EInventoryItemRarity::Mythic:
		return FLinearColor(0.55f, 0.20f, 0.16f, 1.0f);
	default:
		return FLinearColor(0.32f, 0.34f, 0.36f, 1.0f);
	}
}
