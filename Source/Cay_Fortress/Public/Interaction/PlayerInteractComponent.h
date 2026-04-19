// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerInteractComponent.generated.h"

class AActor;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAY_FORTRESS_API UPlayerInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPlayerInteractComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 尝试与当前目标交互 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	/** 获取当前可交互目标 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AActor* GetCurrentInteractable() const { return CurrentInteractable.Get(); }

protected:
	/** 交互检测距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float InteractDistance;

	/** 胶囊半径（球体检测） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TraceRadius;

	/** 是否显示交互调试文本 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bDebugInteraction;

private:
	void UpdateInteractableTarget();
	AActor* FindBestInteractable() const;
	void ShowDebugMessage(const FString& Message, FColor Color = FColor::Yellow, float Duration = 1.5f) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentInteractable;
};
