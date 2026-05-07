// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Enemy/EnemyBehavior.h"
#include "EnemyBehaviorBlueprintLibrary.generated.h"

/**
 * 黑板/行为树与 C++ EEnemyBehavior 的桥接。
 * UE 黑板对 C++ UENUM 的选取器常失败，推荐黑板用 Int + 本库转换。
 */
UCLASS()
class CAY_FORTRESS_API UCayEnemyBehaviorBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Int → EEnemyBehavior（顺序固定，勿改枚举定义顺序）：
	 * 0 Roam, 1 Alert, 2 Chase, 3 Attack；越界则 Roam。
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Enemy")
	static EEnemyBehavior EnemyBehaviorFromIndex(int32 Index);

	UFUNCTION(BlueprintPure, Category = "AI|Enemy")
	static int32 EnemyBehaviorToIndex(EEnemyBehavior Value);

	/**
	 * 将世界坐标投影到 NavMesh（用于黑板 LastKnownLocation，避免 MoveTo 目标不在网上原地发呆）。
	 * QueryExtent 为半范围：XY 水平搜索、Z 垂直容差（角色中心点常略高于地面）。
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Enemy|Nav")
	static bool SnapWorldLocationToNavMesh(
		UObject* WorldContextObject,
		FVector WorldLocation,
		FVector& OutNavLocation,
		float HorizontalQueryHalfExtent = 400.f,
		float VerticalQueryHalfExtent = 600.f);

	/** 优先用 Character 的 NavAgent 采样点，否则用 ActorLocation。 */
	UFUNCTION(BlueprintPure, Category = "AI|Enemy|Nav")
	static FVector GetActorNavSampleLocation(const AActor* Actor);
};
