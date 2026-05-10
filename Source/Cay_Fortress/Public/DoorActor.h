// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DestructibleObstacleInterface.h"
#include "DoorActor.generated.h"

class USphereComponent;

/**
 * 可被敌人破坏的门基类。
 * 门碰撞挖空 NavMesh；破坏后移除碰撞，触发 Dynamic 导航重建。
 * AI 视野可穿透门（Mesh 忽略 ECC_Visibility）。
 */
UCLASS()
class CAY_FORTRESS_API ADoorActor : public AActor, public IDestructibleObstacle
{
	GENERATED_BODY()

public:
	ADoorActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** 敌人检测门的碰撞体（设为 DestructibleObstacle Object Type）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件")
	TObjectPtr<USphereComponent> DoorDetectionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "A | 属性", meta = (ClampMin = "1"))
	float MaxHealth = 150.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "A | 属性")
	float CurrentHealth = 150.f;

	UFUNCTION(BlueprintPure, Category = "A | 状态")
	bool IsDestroyed() const { return bDestroyed; }

	// --- IDestructibleObstacle ---
	virtual bool CanBeDestroyedByAI_Implementation() const override;
	virtual void TakeAIAttackDamage_Implementation(float DamageAmount) override;
	virtual bool IsObstacleDestroyed_Implementation() const override;
	virtual FVector GetObstacleLocation_Implementation() const override;

	/** 每次被攻击时触发（在蓝图中播放受击音效）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "A | 事件")
	void OnDoorTakeDamage(float DamageAmount, float CurrentHealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "A | 事件")
	void OnDoorDestroyed();

protected:
	virtual void BeginPlay() override;

private:
	bool bDestroyed = false;

	void DestroyDoor();
};
