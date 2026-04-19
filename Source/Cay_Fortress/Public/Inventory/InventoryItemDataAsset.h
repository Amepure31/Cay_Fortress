// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/InventoryItemType.h"
#include "Inventory/InventoryItemRarity.h"
#include "Inventory/FItemShapeMask.h"
#include "InventoryItemDataAsset.generated.h"

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Single      UMETA(DisplayName = "单发"),
	FullAuto    UMETA(DisplayName = "全自动")
};

UENUM(BlueprintType)
enum class EWeaponClass : uint8
{
	Pistol      UMETA(DisplayName = "手枪"),
	SMG         UMETA(DisplayName = "冲锋枪"),
	Rifle       UMETA(DisplayName = "步枪"),
	Shotgun     UMETA(DisplayName = "霰弹枪"),
	Sniper      UMETA(DisplayName = "狙击枪"),
	LMG         UMETA(DisplayName = "机枪"),
	Other       UMETA(DisplayName = "其他")
};

UENUM(BlueprintType)
enum class EArmorEquipSlot : uint8
{
	Head        UMETA(DisplayName = "头部"),
	Chest       UMETA(DisplayName = "胸部"),
	Arms        UMETA(DisplayName = "手臂"),
	Legs        UMETA(DisplayName = "腿部"),
	FullBody    UMETA(DisplayName = "全身")
};

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	Pistol      UMETA(DisplayName = "手枪弹"),
	Rifle       UMETA(DisplayName = "步枪弹"),
	Shotgun     UMETA(DisplayName = "霰弹"),
	Sniper      UMETA(DisplayName = "狙击弹"),
	Energy      UMETA(DisplayName = "能量弹"),
	Other       UMETA(DisplayName = "其他")
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FWeaponItemStats
{
	GENERATED_BODY()

	/** 单发伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "伤害", ClampMin = "0.0", UIMin = "0.0"))
	float Damage;

	/** 射速（每秒发射次数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "射速", ClampMin = "0.0", UIMin = "0.0"))
	float FireRate;

	/** 弹夹容量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "弹夹容量", ClampMin = "1", UIMin = "1"))
	int32 MagazineCapacity;

	/** 水平后坐力 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "水平后坐力", ClampMin = "0.0", UIMin = "0.0"))
	float HorizontalRecoil;

	/** 垂直后坐力 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "垂直后坐力", ClampMin = "0.0", UIMin = "0.0"))
	float VerticalRecoil;

	/** 射程（米） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "射程", ClampMin = "0.0", UIMin = "0.0"))
	float Range;

	/** 开火模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "开火模式"))
	EWeaponFireMode FireMode;

	/** 换弹时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "换弹时间", ClampMin = "0.0", UIMin = "0.0"))
	float ReloadTime;

	/** 腰射精度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "腰射精度", ClampMin = "0.0", UIMin = "0.0"))
	float HipFireAccuracy;

	/** 开镜精度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "开镜精度", ClampMin = "0.0", UIMin = "0.0"))
	float ADSAccuracy;

	/** 所属枪械类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "枪械类型"))
	EWeaponClass WeaponClass;

	/** 操控属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (DisplayName = "操控属性", ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Handling;

	FWeaponItemStats()
		: Damage(10.0f)
		, FireRate(4.0f)
		, MagazineCapacity(30)
		, HorizontalRecoil(1.0f)
		, VerticalRecoil(1.0f)
		, Range(100.0f)
		, FireMode(EWeaponFireMode::Single)
		, ReloadTime(2.0f)
		, HipFireAccuracy(50.0f)
		, ADSAccuracy(75.0f)
		, WeaponClass(EWeaponClass::Rifle)
		, Handling(50.0f)
	{
	}
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FFoodItemStats
{
	GENERATED_BODY()

	/** 饱食恢复值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food", meta = (DisplayName = "饱食恢复"))
	float HungerRestore;

	/** 水分恢复值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food", meta = (DisplayName = "水分恢复"))
	float HydrationRestore;

	FFoodItemStats()
		: HungerRestore(10.0f)
		, HydrationRestore(0.0f)
	{
	}
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FMedicalItemStats
{
	GENERATED_BODY()

	/** 总回复量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medical", meta = (DisplayName = "总回复量", ClampMin = "0.0", UIMin = "0.0"))
	float TotalRecoveryAmount;

	/** 恢复速度（每秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medical", meta = (DisplayName = "恢复速度", ClampMin = "0.0", UIMin = "0.0"))
	float RecoveryRate;

	/** 使用时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medical", meta = (DisplayName = "使用时间", ClampMin = "0.0", UIMin = "0.0"))
	float UseTime;

	/** 能否恢复流血/燃烧/感染 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medical", meta = (DisplayName = "可恢复异常状态"))
	bool bCanCureStatusEffects;

	FMedicalItemStats()
		: TotalRecoveryAmount(20.0f)
		, RecoveryRate(10.0f)
		, UseTime(2.0f)
		, bCanCureStatusEffects(false)
	{
	}
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FArmorItemStats
{
	GENERATED_BODY()

	/** 装备部位 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor", meta = (DisplayName = "装备部位"))
	EArmorEquipSlot EquipSlot;

	/** 护甲值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor", meta = (DisplayName = "护甲值", ClampMin = "0.0", UIMin = "0.0"))
	float ArmorValue;

	/** 减伤比例（0-1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor", meta = (DisplayName = "减伤比例", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DamageReductionRatio;

	/** 噪音系数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor", meta = (DisplayName = "噪音系数", ClampMin = "0.0", UIMin = "0.0"))
	float NoiseFactor;

	FArmorItemStats()
		: EquipSlot(EArmorEquipSlot::Chest)
		, ArmorValue(25.0f)
		, DamageReductionRatio(0.15f)
		, NoiseFactor(1.0f)
	{
	}
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FEnergyItemStats
{
	GENERATED_BODY()

	/** 能量值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy", meta = (DisplayName = "能量值", ClampMin = "0.0", UIMin = "0.0"))
	float EnergyValue;

	FEnergyItemStats()
		: EnergyValue(15.0f)
	{
	}
};

USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FAmmoItemStats
{
	GENERATED_BODY()

	/** 子弹类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (DisplayName = "子弹类型"))
	EAmmoType AmmoType;

	/** 穿甲等级 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (DisplayName = "穿甲等级", ClampMin = "0", UIMin = "0"))
	int32 PenetrationLevel;

	/** 是否造成流血 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (DisplayName = "造成流血"))
	bool bCauseBleeding;

	/** 是否造成燃烧 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (DisplayName = "造成燃烧"))
	bool bCauseBurning;

	/** 是否造成感染 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (DisplayName = "造成感染"))
	bool bCauseInfection;

	FAmmoItemStats()
		: AmmoType(EAmmoType::Rifle)
		, PenetrationLevel(0)
		, bCauseBleeding(false)
		, bCauseBurning(false)
		, bCauseInfection(false)
	{
	}
};

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "物品名称"))
	FText ItemName;

	/** 物品描述 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "物品描述"))
	FText Description;

	/** 物品图标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "物品图标"))
	TObjectPtr<UTexture2D> Icon;

	/** 物品类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "物品类型"))
	EInventoryItemType ItemType;

	/** 物品稀有度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "物品稀有度"))
	EInventoryItemRarity Rarity;

	/** 重量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "重量", ClampMin = "0.0", UIMin = "0.0"))
	float Weight;

	/** 价值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "价值", ClampMin = "0", UIMin = "0"))
	int32 Value;

	/** 物品占用宽度（格子数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Size", meta = (DisplayName = "占用宽度", ClampMin = "1", UIMin = "1"))
	int32 Width;

	/** 物品占用高度（格子数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Size", meta = (DisplayName = "占用高度", ClampMin = "1", UIMin = "1"))
	int32 Height;

	/** 物品形状掩码数据（用于不规则形状） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Shape", meta = (DisplayName = "形状掩码数据"))
	TArray<uint8> ShapeMaskData;

	/** 最大堆叠数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stack", meta = (DisplayName = "最大堆叠数量", ClampMin = "1", UIMin = "1"))
	int32 MaxStackSize;

	/** 是否可堆叠 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stack", meta = (DisplayName = "可堆叠"))
	bool bCanStack;

	/** 是否启用耐久度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability", meta = (DisplayName = "启用耐久度"))
	bool bUseDurability;

	/** 最大耐久度（启用耐久度时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability", meta = (DisplayName = "最大耐久度", ClampMin = "1", UIMin = "1", EditCondition = "bUseDurability", EditConditionHides))
	int32 MaxDurability;

	/** 武器专属属性（仅武器显示） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|TypeSpecific", meta = (DisplayName = "武器属性", EditCondition = "ItemType == EInventoryItemType::Weapon", EditConditionHides))
	FWeaponItemStats WeaponStats;

	/** 食物专属属性（仅食物显示） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|TypeSpecific", meta = (DisplayName = "食物属性", EditCondition = "ItemType == EInventoryItemType::Food", EditConditionHides))
	FFoodItemStats FoodStats;

	/** 医疗专属属性（仅医疗显示） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|TypeSpecific", meta = (DisplayName = "医疗属性", EditCondition = "ItemType == EInventoryItemType::Medical", EditConditionHides))
	FMedicalItemStats MedicalStats;

	/** 护甲专属属性（仅护甲显示） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|TypeSpecific", meta = (DisplayName = "护甲属性", EditCondition = "ItemType == EInventoryItemType::Armor", EditConditionHides))
	FArmorItemStats ArmorStats;

	/** 能源专属属性（仅能源显示） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|TypeSpecific", meta = (DisplayName = "能源属性", EditCondition = "ItemType == EInventoryItemType::Energy", EditConditionHides))
	FEnergyItemStats EnergyStats;

	/** 弹药专属属性（仅弹药显示） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|TypeSpecific", meta = (DisplayName = "弹药属性", EditCondition = "ItemType == EInventoryItemType::Ammo", EditConditionHides))
	FAmmoItemStats AmmoStats;

	FInventoryItemData()
		: ItemName(FText::GetEmpty())
		, Description(FText::GetEmpty())
		, Icon(nullptr)
		, ItemType(EInventoryItemType::Other)
		, Rarity(EInventoryItemRarity::Normal)
		, Weight(1.0f)
		, Value(0)
		, Width(1)
		, Height(1)
		, MaxStackSize(1)
		, bCanStack(false)
		, bUseDurability(false)
		, MaxDurability(100)
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (DisplayName = "物品数据"))
	FInventoryItemData ItemData;
	
};
