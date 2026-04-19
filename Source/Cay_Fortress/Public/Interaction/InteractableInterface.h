#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class CAY_FORTRESS_API UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可交互接口
 * 任何可被玩家交互的对象（容器、门、终端等）都可实现该接口
 */
class CAY_FORTRESS_API IInteractableInterface
{
	GENERATED_BODY()

public:
	/** 是否允许当前交互者进行交互 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	/** 执行交互行为 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);

	/** 获取交互提示文本（例如：按E打开容器） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractText() const;
};

