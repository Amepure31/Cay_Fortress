// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "Inventory/FItemShapeMask.h"
#include "FInventoryItemInstance.generated.h"

/** 武器属性覆写（工作台改装写入的每实例增量数据） */
USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FWeaponStatOverrides
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideDamage = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideDamage"))
	float Damage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideFireRate = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideFireRate"))
	float FireRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideMagazineCapacity = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideMagazineCapacity"))
	int32 MagazineCapacity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideHorizontalRecoil = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideHorizontalRecoil"))
	float HorizontalRecoil = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideVerticalRecoil = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideVerticalRecoil"))
	float VerticalRecoil = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideRange = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideRange"))
	float Range = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideReloadTime = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideReloadTime"))
	float ReloadTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideHipFireAccuracy = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideHipFireAccuracy"))
	float HipFireAccuracy = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideADSAccuracy = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideADSAccuracy"))
	float ADSAccuracy = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	bool bOverrideHandling = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (EditCondition = "bOverrideHandling"))
	float Handling = 0.0f;
};

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

	/** 旋转角度（顺时针，单位90度） */
	UPROPERTY()
	int32 RotationQuarterTurns;

	/** 绑定时间 */
	UPROPERTY()
	FDateTime BindTime;

	/** 是否已绑定（绑定后不可交易） */
	UPROPERTY()
	bool bIsBound;

	/** 武器当前弹匣内子弹数（仅 ItemType==Weapon 有意义；换弹与开火由角色逻辑维护） */
	UPROPERTY()
	int32 WeaponMagazineAmmo;

	/** 容器背包内的物品网格（仅当 ItemData.ArmorStats.bIsContainer 为 true 时延迟创建）。物品实例被 GC 时一并回收。 */
	UPROPERTY()
	TObjectPtr<class UInventoryComponent> ContainerInventory;

	/** 每实例武器属性覆写（由武器工作台写入）。仅 ItemType==Weapon 时有意义。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon")
	FWeaponStatOverrides WeaponStatOverrides;

	/** 武器改装等级 (Key = (uint8)EWeaponModType, Value = 当前等级) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon")
	TMap<uint8, int32> WeaponModLevels;

	/** 确保容器背包网格存在并以指定尺寸初始化（非容器物品为 no-op） */
	void EnsureContainerInventory(int32 InGridWidth, int32 InGridHeight);

	/** 合并基础 DataAsset 属性与覆写，返回有效武器属性 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Weapon")
	FWeaponItemStats GetEffectiveWeaponStats() const;

	UInventoryItemInstance()
		: ItemData(nullptr)
		, StackSize(1)
		, Durability(0)
		, MaxDurability(0)
		, SlotX(-1)
		, SlotY(-1)
		, Width(1)
		, Height(1)
		, RotationQuarterTurns(0)
		, BindTime(FDateTime::MinValue())
		, bIsBound(false)
		, WeaponMagazineAmmo(0)
		, ContainerInventory(nullptr)
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
			Width = InItemData->ItemData.Width;
			Height = InItemData->ItemData.Height;
			ShapeMask.Init(Width, Height, InItemData->ItemData.ShapeMaskData);
			RotationQuarterTurns = 0;

			if (InItemData->ItemData.bUseDurability)
			{
				MaxDurability = FMath::Max(1, InItemData->ItemData.MaxDurability);
				Durability = MaxDurability;
			}
			else
			{
				MaxDurability = 0;
				Durability = 0;
			}

			if (InItemData->ItemData.ItemType == EInventoryItemType::Weapon)
			{
				WeaponMagazineAmmo = FMath::Max(0, InItemData->ItemData.WeaponStats.MagazineCapacity);
			}
			else
			{
				WeaponMagazineAmmo = 0;
			}
		}
	}
};
