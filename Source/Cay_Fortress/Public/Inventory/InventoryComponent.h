#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/FItemShapeMask.h"
#include "Inventory/FInventoryGridCell.h"
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
	 * 移除物品
	 * @param ItemInstance 物品实例
	 * @return 是否移除成功
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(class UInventoryItemInstance* ItemInstance);

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

private:
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
};