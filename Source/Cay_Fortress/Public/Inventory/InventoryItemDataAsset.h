// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/InventoryItemRarity.h"
#include "Inventory/FItemShapeMask.h"
#include "InventoryItemDataAsset.generated.h"

/**
 * 物品数据结构体
 * 用于存储物品的静态配置数据
 * 在DataAsset中定义，可在蓝图中配置
 */
USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FInventoryItemData
{
	GENERATED_BODY()

	/** 物品名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;

	/** 物品描述 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	/** 物品图标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	/** 物品类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EInventoryItemType ItemType;

	/** 物品稀有度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EInventoryItemRarity Rarity;

	/** 物品占用宽度（格子数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Size", meta = (ClampMin = "1", UIMin = "1"))
	int32 Width;

	/** 物品占用高度（格子数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Size", meta = (ClampMin = "1", UIMin = "1"))
	int32 Height;

	/** 物品形状掩码数据（用于不规则形状） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Shape")
	TArray<uint8> ShapeMaskData;

	/** 最大堆叠数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stack", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxStackSize;

	/** 是否可堆叠 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stack")
	bool bCanStack;

	FInventoryItemData()
		: ItemName(FText::GetEmpty())
		, Description(FText::GetEmpty())
		, Icon(nullptr)
		, ItemType(EInventoryItemType::Other)
		, Rarity(EInventoryItemRarity::Normal)
		, Width(1)
		, Height(1)
		, MaxStackSize(1)
		, bCanStack(false)
	{
		ShapeMaskData.Add(1);
	}
};

/**
 * 物品数据资产
 * 用于在编辑器中创建和配置物品数据
 * 可在蓝图中引用和访问
 */
UCLASS()
class CAY_FORTRESS_API UInventoryItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** 物品数据 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FInventoryItemData ItemData;
	
};
