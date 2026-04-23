// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootContainer/ContainerType.h"
#include "RoomActor.generated.h"

class ALootContainerActor;
class AContainerGenerationTag;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomContainersOpenedStateChanged, bool, bAllContainersOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomContainersLootedStateChanged, bool, bAllContainersLooted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomContainerItemRefreshRequested, ALootContainerActor*, Container);

UCLASS()
class CAY_FORTRESS_API ARoomActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ARoomActor();

protected:
	virtual void BeginPlay() override;
	virtual void PostLoad() override;

public:	
	/** 已弃用：现在固定只使用附属挂载方式收集容器 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Auto-collect attached containers is always enabled now."))
	bool bAutoCollectAttachedContainers;

	/** BeginPlay时是否自动执行一次容器刷新判定（决定容器本体是否存在） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room|Container Refresh")
	bool bRefreshContainersOnBeginPlay;

	/** 所有容器默认刷新概率（0~1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room|Container Refresh", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DefaultContainerRefreshChance;

	/** 按容器类型覆盖刷新概率（未配置的类型使用默认概率） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room|Container Refresh")
	TMap<EContainerType, float> ContainerRefreshChanceByType;

	/** 已弃用：手动绑定容器方式不再使用，仅保留为兼容旧蓝图数据 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Room|Compatibility", meta = (DeprecatedProperty, DeprecationMessage = "Manual container binding is deprecated. Use attached LootContainerActor only."))
	TArray<TObjectPtr<ALootContainerActor>> ManagedContainers;

	// ---- Backward compatibility for existing BP assets ----
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use bAutoCollectAttachedContainers instead."))
	bool bAutoCollectAttachedTags_DEPRECATED;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use bRefreshContainersOnBeginPlay instead."))
	bool bGenerateContainersOnBeginPlay_DEPRECATED;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Second refresh now triggers automatically after phase one succeeds."))
	bool bRefreshContainerItemsOnBeginPlay_DEPRECATED;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Container tag pipeline is deprecated."))
	TSubclassOf<ALootContainerActor> DefaultContainerClass_DEPRECATED;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use ManagedContainers/attached LootContainerActors instead."))
	TArray<TObjectPtr<AContainerGenerationTag>> ContainerGenerationTags_DEPRECATED;

	/** 房间所有容器是否都被打开过 */
	UPROPERTY(BlueprintReadOnly, Category = "Room|State")
	bool bAllContainersOpened;

	/** 房间所有容器是否都被搜刮完（库存为空） */
	UPROPERTY(BlueprintReadOnly, Category = "Room|State")
	bool bAllContainersLooted;

	/** 状态变化事件：是否全部打开 */
	UPROPERTY(BlueprintAssignable, Category = "Room|Events")
	FOnRoomContainersOpenedStateChanged OnAllContainersOpenedStateChanged;

	/** 状态变化事件：是否全部搜刮 */
	UPROPERTY(BlueprintAssignable, Category = "Room|Events")
	FOnRoomContainersLootedStateChanged OnAllContainersLootedStateChanged;

	/** 容器物品刷新请求（仅对已激活容器广播） */
	UPROPERTY(BlueprintAssignable, Category = "Room|Events")
	FOnRoomContainerItemRefreshRequested OnContainerItemRefreshRequested;

	/** 手动刷新房间容器列表 */
	UFUNCTION(BlueprintCallable, Category = "Room|Container Refresh")
	void CollectManagedContainers();

	/** 执行容器刷新判定（控制容器本体是否激活） */
	UFUNCTION(BlueprintCallable, Category = "Room|Container Refresh")
	void RefreshContainerPresence();

	UFUNCTION(BlueprintCallable, Category = "Room|Compatibility", meta = (DeprecatedFunction, DeprecationMessage = "Use CollectManagedContainers instead."))
	void CollectContainerGenerationTags();

	UFUNCTION(BlueprintCallable, Category = "Room|Compatibility", meta = (DeprecatedFunction, DeprecationMessage = "Use RefreshContainerPresence instead."))
	void GenerateContainers();

	/** 执行容器内物品刷新阶段（仅对已激活容器生效） */
	UFUNCTION(BlueprintCallable, Category = "Room|Container Refresh")
	void RefreshItemsInActiveContainers();

	/** 重新计算房间状态 */
	UFUNCTION(BlueprintCallable, Category = "Room|State")
	void EvaluateRoomState();

	/** 当前房间是否全部打开 */
	UFUNCTION(BlueprintCallable, Category = "Room|State")
	bool AreAllContainersOpened() const { return bAllContainersOpened; }

	/** 当前房间是否全部搜刮 */
	UFUNCTION(BlueprintCallable, Category = "Room|State")
	bool AreAllContainersLooted() const { return bAllContainersLooted; }

protected:
	UFUNCTION()
	void OnTrackedContainerInventoryChanged();

	UFUNCTION()
	void OnTrackedContainerInventoryToggled(bool bIsOpen);

	bool IsContainerInventoryEmpty(const ALootContainerActor* Container) const;
	void RegisterContainerForStateTracking(ALootContainerActor* Container);
	void SetContainerActiveState(ALootContainerActor* Container, bool bActive);
	float GetRefreshChanceForContainerType(EContainerType ContainerType) const;
	void RequestContainerItemRefresh(ALootContainerActor* Container);
	void TriggerSecondRefreshAfterFirstPhase();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALootContainerActor>> CollectedAttachedContainers;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ALootContainerActor>> ActiveContainers;

	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<ALootContainerActor>> EverOpenedContainers;

	UPROPERTY(Transient)
	bool bHasShownAllContainersOpenedPrompt;

};
