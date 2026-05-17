// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerInteractComponent.generated.h"

class AActor;
class UWidgetComponent;

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

	/** 球体检测半径 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TraceRadius;

	/** 是否显示交互调试文本 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bDebugInteraction;

	/** 交互提示控件类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Prompt")
	TSubclassOf<UUserWidget> InteractPromptWidgetClass;

	/** 提示相对目标的位置偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Prompt")
	FVector InteractPromptOffset = FVector(0, 0, 120.0f);

private:
	void UpdateInteractableTarget();
	AActor* FindBestInteractable() const;
	void ShowDebugMessage(const FString& Message, FColor Color = FColor::Yellow, float Duration = 1.5f) const;
	void UpdateInteractPrompt();

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentInteractable;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> InteractPromptComp;
};
