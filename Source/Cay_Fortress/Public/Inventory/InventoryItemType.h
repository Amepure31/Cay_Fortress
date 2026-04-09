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
	Weapon         UMETA(DisplayName = "Weapon"),
	
	/** 护甲 */
	Armor          UMETA(DisplayName = "Armor"),
	
	/** 消耗品 */
	Consumable     UMETA(DisplayName = "Consumable"),
	
	/** 材料 */
	Material       UMETA(DisplayName = "Material"),
	
	/** 制作配方 */
	Recipe         UMETA(DisplayName = "Recipe"),
	
	/** 任务物品 */
	Quest          UMETA(DisplayName = "Quest"),
	
	/** 其他 */
	Other          UMETA(DisplayName = "Other")
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
