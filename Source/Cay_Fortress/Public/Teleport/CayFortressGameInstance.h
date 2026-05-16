#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CayFortressGameInstance.generated.h"

UCLASS()
class CAY_FORTRESS_API UCayFortressGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 目标传送节点ID，跨关卡保存。NAME_None 表示无待处理传送。 */
	UPROPERTY(BlueprintReadWrite, Category = "Teleport")
	FName PendingTeleportNodeID;

	/** 来源关卡名，预留。 */
	UPROPERTY(BlueprintReadWrite, Category = "Teleport")
	FName LastLevelName;
};
