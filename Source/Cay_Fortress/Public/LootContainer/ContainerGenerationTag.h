// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootContainer/ContainerType.h"
#include "ContainerGenerationTag.generated.h"

class ALootContainerActor;

UCLASS()
class CAY_FORTRESS_API AContainerGenerationTag : public AActor
{
	GENERATED_BODY()
	
public:	
	AContainerGenerationTag();

	/** 该点位是否参与容器生成 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Spawn")
	bool bEnableSpawn;

	/** 该点位指定的容器类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Spawn")
	EContainerType ContainerType;

	/** 该点位使用的容器蓝图类（为空时由Room使用默认类） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Spawn")
	TSubclassOf<ALootContainerActor> ContainerClassOverride;

	/** 获取最终要生成的容器类（优先使用点位覆盖类） */
	UFUNCTION(BlueprintCallable, Category = "Container Spawn")
	TSubclassOf<ALootContainerActor> ResolveContainerClass(TSubclassOf<ALootContainerActor> FallbackClass) const;

	/** 是否允许在该点位生成容器 */
	UFUNCTION(BlueprintCallable, Category = "Container Spawn")
	bool CanGenerateContainer() const { return bEnableSpawn; }

	/** 获取该点位已生成的容器 */
	UFUNCTION(BlueprintCallable, Category = "Container Spawn")
	ALootContainerActor* GetSpawnedContainer() const { return SpawnedContainer.Get(); }

	/** 由Room在生成后回写引用 */
	void SetSpawnedContainer(ALootContainerActor* InContainer);

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<ALootContainerActor> SpawnedContainer;

};
