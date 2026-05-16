#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "TeleportNode.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;
class USphereComponent;

UCLASS()
class CAY_FORTRESS_API ATeleportNode : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ATeleportNode();

	// -- 配置 --

	/** 目标关卡名（如 "TestMap2"） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FName TargetLevelName;

	/** 当前节点在本关卡中的唯一ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FName TeleportNodeID;

	/** 目标关卡中对应节点的ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FName TargetTeleportNodeID;

	/** 交互提示文本 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FText InteractText;

	// -- 组件 --

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Visuals")
	TObjectPtr<UStaticMeshComponent> NodeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Visuals")
	TObjectPtr<UParticleSystemComponent> TeleportVisuals;

	/** 交互碰撞体，阻挡可见性通道以便交互扫射线检测到 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<USphereComponent> InteractionVolume;

	/** 玩家传送到达点。在编辑器中拖拽此组件设置到达位置和朝向 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<USceneComponent> ArrivalPoint;

	// -- IInteractableInterface --

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractText_Implementation() const override;
};
