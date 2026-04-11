// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "FItemShapeMask.generated.h"

/**
 * 物品形状掩码结构体
 * 用于表示不规则形状的物品占用的格子
 * 使用二维布尔数组表示物品占用情况
 */
USTRUCT(BlueprintType)
struct CAY_FORTRESS_API FFItemShapeMask
{
	GENERATED_BODY()

	/** 物品占用的格子矩阵（true表示占用，false表示空闲） */
	UPROPERTY(BlueprintReadWrite, Category = "Shape")
	TArray<uint8> ShapeMaskData;

	/** 物品占用宽度（格子数） */
	UPROPERTY(BlueprintReadWrite, Category = "Shape")
	int32 Width;

	/** 物品占用高度（格子数） */
	UPROPERTY(BlueprintReadWrite, Category = "Shape")
	int32 Height;

	FFItemShapeMask()
		: Width(1)
		, Height(1)
	{
		ShapeMaskData.Add(1);
	}

	/**
	 * 初始化形状掩码
	 * @param InWidth 物品宽度
	 * @param InHeight 物品高度
	 * @param InShapeMask 形状掩码数据（按行排列，1表示占用，0表示空闲）
	 */
	void Init(int32 InWidth, int32 InHeight, const TArray<uint8>& InShapeMask)
	{
		Width = InWidth;
		Height = InHeight;
		ShapeMaskData = InShapeMask;
	}

	/**
	 * 检查指定位置是否被占用
	 * @param X 局部X坐标
	 * @param Y 局部Y坐标
	 * @return 是否被占用
	 */
	bool IsOccupied(int32 X, int32 Y) const
	{
		if (X >= 0 && X < Width && Y >= 0 && Y < Height)
		{
			int32 Index = Y * Width + X;
			if (Index >= 0 && Index < ShapeMaskData.Num())
			{
				return ShapeMaskData[Index] != 0;
			}
		}
		return false;
	}

	/**
	 * 获取占用的格子总数
	 */
	int32 GetOccupiedCellCount() const
	{
		int32 Count = 0;
		for (uint8 Value : ShapeMaskData)
		{
			if (Value != 0)
			{
				Count++;
			}
		}
		return Count;
	}
};