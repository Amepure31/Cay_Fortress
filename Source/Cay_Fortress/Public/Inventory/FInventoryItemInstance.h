// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FItemShapeMask.h"
#include "FInventoryItemInstance.generated.h"

/**
 * 物品实例类
 * 用于存储单个物品的动态数据
 * 由InventoryComponent管理
 */
UCLASS()
class CAY_FORTRESS_API UInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	/** 物品数据模板类 */
	UPROPERTY()
	TSubclassOf<UInventoryItemDataAsset> ItemClass;

	/** 物品数据指针 */
	UPROPERTY()
	UInventoryItemDataAsset* ItemData;

	/** 物品数量 */
	UPROPERTY()
	int32 StackSize;

	/** 当前耐久度 */
	UPROPERTY()
	int32 Durability;

	/** 最大耐久度 */
	UPROPERTY()
	int32 MaxDurability;

	/** 在背包中的X坐标位置 */
	UPROPERTY()
	int32 SlotX;

	/** 在背包中的Y坐标位置 */
	UPROPERTY()
	int32 SlotY;

	/** 物品占用宽度（格子数） */
	UPROPERTY()
	int32 Width;

	/** 物品占用高度（格子数） */
	UPROPERTY()
	int32 Height;

	/** 物品形状掩码 */
	UPROPERTY()
	FFItemShapeMask ShapeMask;

	/** 绑定时间 */
	UPROPERTY()
	FDateTime BindTime;

	/** 是否已绑定（绑定后不可交易） */
	UPROPERTY()
	bool bIsBound;

	UInventoryItemInstance()
		: ItemClass(nullptr)
		, ItemData(nullptr)
		, StackSize(1)
		, Durability(100)
		, MaxDurability(100)
		, SlotX(-1)
		, SlotY(-1)
		, Width(1)
		, Height(1)
		, BindTime(FDateTime::MinValue())
		, bIsBound(false)
	{
	}

	/**
	 * 设置物品数据
	 * @param InItemData 物品数据资产指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemData(UInventoryItemDataAsset* InItemData)
	{
		ItemData = InItemData;
		if (ItemData)
		{
			ItemClass = InItemData->GetClass();
			Width = InItemData->ItemData.Width;
			Height = InItemData->ItemData.Height;
			ShapeMask.Init(Width, Height, InItemData->ItemData.ShapeMaskData);
			MaxDurability = InItemData->ItemData.bCanStack ? 1 : 100;
			Durability = MaxDurability;
		}
	}
};
