// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "FInventoryItemInstance.h"
#include "FInventoryGridCell.generated.h"

/**
 * 背包网格单元结构体
 * 用于表示背包中单个格子的状态
 */
USTRUCT()
struct CAY_FORTRESS_API FFInventoryGridCell
{
	GENERATED_BODY()

	/** 是否被占用 */
	UPROPERTY()
	bool bOccupied;

	/** 占用的物品实例指针 */
	UPROPERTY()
	UInventoryItemInstance* ItemInstance;

	/** 物品占用宽度（格子数） */
	UPROPERTY()
	int32 ItemWidth;

	/** 物品占用高度（格子数） */
	UPROPERTY()
	int32 ItemHeight;

	/** 物品起始X坐标 */
	UPROPERTY()
	int32 ItemSlotX;

	/** 物品起始Y坐标 */
	UPROPERTY()
	int32 ItemSlotY;

	FFInventoryGridCell()
		: bOccupied(false)
		, ItemInstance(nullptr)
		, ItemWidth(1)
		, ItemHeight(1)
		, ItemSlotX(-1)
		, ItemSlotY(-1)
	{
	}

	/**
	 * 设置格子为占用状态
	 * @param InItemInstance 物品实例指针
	 * @param InItemWidth 物品占用宽度
	 * @param InItemHeight 物品占用高度
	 * @param InSlotX 物品起始X坐标
	 * @param InSlotY 物品起始Y坐标
	 */
	void SetOccupied(UInventoryItemInstance* InItemInstance, int32 InItemWidth, int32 InItemHeight, int32 InSlotX, int32 InSlotY)
	{
		bOccupied = true;
		ItemInstance = InItemInstance;
		ItemWidth = InItemWidth;
		ItemHeight = InItemHeight;
		ItemSlotX = InSlotX;
		ItemSlotY = InSlotY;
	}

	/**
	 * 清空格子状态
	 */
	void Clear()
	{
		bOccupied = false;
		ItemInstance = nullptr;
		ItemWidth = 1;
		ItemHeight = 1;
		ItemSlotX = -1;
		ItemSlotY = -1;
	}
};
