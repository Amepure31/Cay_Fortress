// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "InventoryItemType.generated.h"

/**
 * 物品类型枚举
 */
UENUM()
enum class EInventoryItemType : uint8
{
	/** 武器 */
	Weapon                  = 0 UMETA(DisplayName = "武器"),
	
	/** 护甲 */
	Armor                   = 1 UMETA(DisplayName = "护甲"),

	/** 兼容旧资产：消耗品（已弃用） */
	Consumable_DEPRECATED   = 2 UMETA(DisplayName = "消耗品(已弃用)", Hidden),
	
	/** 材料 */
	Material                = 3 UMETA(DisplayName = "材料"),
	
	/** 制作配方 */
	Recipe                  = 4 UMETA(DisplayName = "制作配方"),
	
	/** 任务物品 */
	Quest                   = 5 UMETA(DisplayName = "任务物品"),
	
	/** 其他 */
	Other                   = 6 UMETA(DisplayName = "其他"),

	/** 食物 */
	Food                    = 7 UMETA(DisplayName = "食物"),

	/** 医疗 */
	Medical                 = 8 UMETA(DisplayName = "医疗"),

	/** 能源 */
	Energy                  = 9 UMETA(DisplayName = "能源"),

	/** 弹药 */
	Ammo                    = 10 UMETA(DisplayName = "弹药"),

	/** 枚举上限（隐藏） */
	MAX                     = 11 UMETA(Hidden)
};

/**
 * 物品类型枚举类
 * 用于UE反射系统
 */
UCLASS()
class CAY_FORTRESS_API UInventoryItemType : public UEnum
{
	GENERATED_BODY()
	
};
