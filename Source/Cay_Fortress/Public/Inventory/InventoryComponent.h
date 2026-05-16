#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/FItemShapeMask.h"
#include "Inventory/FInventoryGridCell.h"
#include "Inventory/InventoryItemDataAsset.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryToggledDelegate, bool, bIsOpen);

/**
 * 背包组件
 * 管理背包的二维网格系统，支持不规则物品的放置和移动（拼图式背包）
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAY_FORTRESS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 背包网格宽度（格子数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Grid", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridWidth;

	/** 背包网格高度（格子数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Grid", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridHeight;

	/** 背包是否打开 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|State")
	bool bIsInventoryOpen;

	/** 禁止放入护甲容器物品（用于容器背包内部的 InventoryComponent，阻止嵌套容器） */
	UPROPERTY()
	bool bRejectContainerItems;

	/** 二维网格数组 */
	UPROPERTY()
	TArray<FFInventoryGridCell> Grid;

	/** 物品列表 */
	UPROPERTY()
	TArray<TObjectPtr<class UInventoryItemInstance>> Items;

	/** 背包变化委托 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChangedDelegate OnInventoryChanged;

	/** 背包打开/关闭状态变化委托 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryToggledDelegate OnInventoryToggled;

	/**
	 * 初始化背包网格
	 * @param InWidth 网格宽度
	 * @param InHeight 网格高度
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeGrid(int32 InWidth, int32 InHeight);

	/**
	 * 清空背包
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Clear();

	/**
	 * 打开背包
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenInventory();

	/**
	 * 关闭背包
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();

	/**
	 * 切换背包状态（打开/关闭）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	/**
	 * 检查背包是否打开
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsInventoryOpen() const { return bIsInventoryOpen; }

	/**
	 * 添加物品到背包
	 * @param ItemData 物品数据资产
	 * @param StackSize 堆叠数量
	 * @return 是否添加成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(class UInventoryItemDataAsset* ItemData, int32 StackSize = 1);

	/**
	 * 在指定位置添加物品
	 * @param ItemData 物品数据资产
	 * @param SlotX 起始X位置
	 * @param SlotY 起始Y位置
	 * @param StackSize 堆叠数量
	 * @return 是否添加成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemAtPosition(class UInventoryItemDataAsset* ItemData, int32 SlotX, int32 SlotY, int32 StackSize = 1);

	/**
	 * 在指定位置按给定形状添加物品实例（用于跨容器转移等场景）
	 */
	class UInventoryItemInstance* AddItemWithShapeAtPosition(
		class UInventoryItemDataAsset* ItemData,
		int32 SlotX,
		int32 SlotY,
		int32 StackSize,
		int32 Width,
		int32 Height,
		const FFItemShapeMask& ShapeMask,
		int32 RotationQuarterTurns,
		int32 Durability = -1,
		int32 MaxDurability = -1,
		bool bIsBound = false,
		FDateTime BindTime = FDateTime::MinValue());

	/**
	 * 仅用于背包 UI「添加物品」按钮：先 AddItem，若为武器且开启入包附带弹药，再按弹匣容量以散装（不并入已有弹药堆）放入弹药。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemFromInventorySpawnerButton(class UInventoryItemDataAsset* ItemData, int32 StackSize = 1);

	/**
	 * 将武器实例弹匣内子弹退为散装弹药（每发尽量单独占格/新堆，不合并已有堆叠）。
	 * @return 是否至少退出一发
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	bool TryUnloadWeaponMagazineToLooseAmmo(class UInventoryItemInstance* WeaponInstance);

	/**
	 * 移除物品
	 * @param ItemInstance 物品实例
	 * @return 是否移除成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(class UInventoryItemInstance* ItemInstance);

	/**
	 * 从网格与 Items 列表移除实例但不销毁（转移到装备栏、另一库存等仍持有同一 UObject 时使用）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DetachItemInstance(class UInventoryItemInstance* ItemInstance);

	/**
	 * 将已有物品实例放回网格（DetachItemInstance 的逆操作）。
	 * 自动寻找可用空位，保留实例上的所有数据（改装覆写等）。
	 * @return 是否放置成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AttachItemInstance(class UInventoryItemInstance* ItemInstance);

	/**
	 * 移除指定位置的物品
	 * @param SlotX X位置
	 * @param SlotY Y位置
	 * @return 被移除的物品实例，如果没有物品返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	class UInventoryItemInstance* RemoveItemAtPosition(int32 SlotX, int32 SlotY);

	/**
	 * 移动物品到新位置
	 * @param ItemInstance 物品实例
	 * @param NewSlotX 新位置X
	 * @param NewSlotY 新位置Y
	 * @return 是否移动成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(class UInventoryItemInstance* ItemInstance, int32 NewSlotX, int32 NewSlotY);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItemWithShape(
		class UInventoryItemInstance* ItemInstance,
		int32 NewSlotX,
		int32 NewSlotY,
		int32 NewWidth,
		int32 NewHeight,
		const FFItemShapeMask& NewShapeMask,
		int32 NewRotationQuarterTurns);

	/**
	 * 检查指定位置是否可以放置物品
	 * @param Width 物品宽度
	 * @param Height 物品高度
	 * @param ShapeMask 物品形状掩码
	 * @param SlotX 起始X位置
	 * @param SlotY 起始Y位置
	 * @param IgnoreItem 忽略的物品实例（用于移动时忽略自身）
	 * @return 是否可以放置
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsSpaceAvailable(int32 Width, int32 Height, const FFItemShapeMask& ShapeMask, int32 SlotX, int32 SlotY, class UInventoryItemInstance* IgnoreItem = nullptr) const;

	/**
	 * 查找可以放置物品的位置
	 * @param Width 物品宽度
	 * @param Height 物品高度
	 * @param ShapeMask 物品形状掩码
	 * @param OutSlotX 输出找到的X位置
	 * @param OutSlotY 输出找到的Y位置
	 * @return 是否找到可用位置
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool FindSpaceForItem(int32 Width, int32 Height, const FFItemShapeMask& ShapeMask, int32& OutSlotX, int32& OutSlotY) const;

	/**
	 * 获取指定位置的物品实例
	 * @param SlotX X位置
	 * @param SlotY Y位置
	 * @return 物品实例，如果没有物品返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	class UInventoryItemInstance* GetItemAtPosition(int32 SlotX, int32 SlotY) const;

	/**
	 * 检查位置是否在网格范围内
	 * @param SlotX X位置
	 * @param SlotY Y位置
	 * @return 是否在范围内
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsValidPosition(int32 SlotX, int32 SlotY) const;

	/** 背包内某弹药类型的总数量（堆叠合计） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Ammo")
	int32 GetTotalAmmoCountByAmmoType(EAmmoType Type) const;

	/**
	 * 与换弹、HUD 备弹统计一致：该武器在背包中应匹配的弹药 EAmmoType（优先 CompanionAmmoItemOverride 上弹药物品的 AmmoType）。
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Ammo")
	EAmmoType GetReserveAmmoTypeForWeapon(const class UInventoryItemDataAsset* WeaponDA) const;

	/**
	 * 背包内可作为当前武器备弹的弹药总数：若武器配置了 Companion 弹药资产，则统计该资产及同 EAmmoType 的弹药堆叠；否则按枪械类型对应 EAmmoType 统计。
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Ammo")
	int32 GetTotalReserveRoundsForWeaponInInventory(const class UInventoryItemDataAsset* WeaponDA) const;

	/** 从背包扣除指定类型弹药，返回实际扣除数量 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Ammo")
	int32 ConsumeAmmoFromInventoryByType(EAmmoType Type, int32 Amount);

	/** 从背包消耗指定 DataAsset 的物品（按堆叠扣减），返回实际扣除数量 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 ConsumeItemByDataAsset(UInventoryItemDataAsset* ItemData, int32 Amount);

	/** 扩容网格（仅扩大，保留已有物品），返回是否成功 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ExpandGrid(int32 NewWidth, int32 NewHeight);

	/** 计算背包中所有物品的总重量（Weight * StackSize 累加） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	float GetTotalCarriedWeight() const;

	/**
	 * 将已有物品实例放入背包空闲格（不新建 UObject，不触发武器「入包附带弹药」）。
	 * 用于卸下装备等场景。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddExistingItemInstance(class UInventoryItemInstance* Item);

	/**
	 * 武器入包附带弹药：当武器资产未指定 CompanionAmmoItemOverride 时，按枪械类型在此表查找默认弹药物品。
	 * 键为 EAmmoType（与 GetExpectedAmmoTypeForWeaponClass 一致）；若整表为空，BeginPlay 会尝试从 /Game 下扫描 UInventoryItemDataAsset 自动填入每种 EAmmoType 的首个弹药资产。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Ammo")
	TMap<EAmmoType, TObjectPtr<class UInventoryItemDataAsset>> DefaultAmmoItemByType;

private:
	mutable bool bDidAutoDiscoverDefaultAmmo = false;

	/**
	 * 占用网格位置
	 * @param ItemInstance 物品实例
	 * @param SlotX 起始X位置
	 * @param SlotY 起始Y位置
	 */
	void OccupyGrid(class UInventoryItemInstance* ItemInstance, int32 SlotX, int32 SlotY);

	/**
	 * 释放网格位置
	 * @param ItemInstance 物品实例
	 */
	void ReleaseGrid(class UInventoryItemInstance* ItemInstance);

	/**
	 * 获取网格索引
	 */
	int32 GetGridIndex(int32 X, int32 Y) const;

	/**
	 * 通知背包变化
	 */
	void NotifyInventoryChanged();

	void TryAutoDiscoverDefaultAmmoItemsIfNeeded();

	/** 与 AddItem 相同逻辑，成功时返回新建或合并到的实例，失败 nullptr */
	class UInventoryItemInstance* TryAddItemReturningInstance(class UInventoryItemDataAsset* ItemData, int32 StackSize = 1);

	class UInventoryItemDataAsset* ResolveCompanionAmmoItemForWeapon(const class UInventoryItemDataAsset* WeaponDA) const;
	void TryGrantCompanionAmmoForWeaponLoose(class UInventoryItemDataAsset* WeaponItemData);
	/**
	 * 散装弹药入包：bAllowStackMerge=true 时可堆叠弹药走 AddItem 合并/分堆；false 时每发单独占格（如长按退弹）。
	 * @return 实际放入发数
	 */
	int32 GrantLooseAmmoAsSingleStacks(class UInventoryItemDataAsset* AmmoDA, int32 RoundCount, bool bAllowStackMerge = true);
	bool TryPlaceSingleNewStackIgnoringMerge(class UInventoryItemDataAsset* ItemData, int32 StackSize);
};