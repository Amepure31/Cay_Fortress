// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "LootContainer/ContainerType.h"
#include "LootContainerActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UInventoryComponent;

UCLASS()
class CAY_FORTRESS_API ALootContainerActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	ALootContainerActor();

protected:
	virtual void BeginPlay() override;

	/** 根组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	/** 容器可视网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ContainerMesh;

	/** 容器库存组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> InventoryComponent;

	/** 容器网格宽度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container|Inventory", meta = (ClampMin = "1", UIMin = "1"))
	int32 ContainerGridWidth;

	/** 容器网格高度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container|Inventory", meta = (ClampMin = "1", UIMin = "1"))
	int32 ContainerGridHeight;

public:
	/** 容器类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	EContainerType ContainerType;

	/** 打开容器（触发库存打开状态） */
	UFUNCTION(BlueprintCallable, Category = "Container")
	void OpenContainer();

	/** 关闭容器（触发库存关闭状态） */
	UFUNCTION(BlueprintCallable, Category = "Container")
	void CloseContainer();

	/** 当前容器是否已打开 */
	UFUNCTION(BlueprintCallable, Category = "Container")
	bool IsContainerOpen() const;

	/** 获取容器库存组件 */
	UFUNCTION(BlueprintCallable, Category = "Container")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** IInteractableInterface */
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractText_Implementation() const override;

};
