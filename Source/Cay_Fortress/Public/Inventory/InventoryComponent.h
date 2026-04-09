// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/FItemShapeMask.h"
#include "Inventory/FInventoryGridCell.h"
#include "InventoryComponent.generated.h"

/**
 * 背包组件
 * 管理背包的二维网格系统，支持不规则物品的放置和移动
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

	/** 二维网格数组 */
	UPROPERTY()
	TArray<FFInventoryGridCell> Grid;

	/** 物品列表 */
	UPROPERTY()
	TArray<TObjectPtr<class UInventoryItemInstance>> Items;

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

private:
};
