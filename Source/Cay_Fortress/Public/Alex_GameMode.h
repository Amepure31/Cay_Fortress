// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Alex_GameMode.generated.h"

class AAlex_PlayerCharacter;
class AAlex_PlayerController;
class AAlex_HUD;

UCLASS()
class CAY_FORTRESS_API AAlex_GameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AAlex_GameMode();

	virtual void BeginPlay() override;

protected:
	/** 关卡加载后将玩家移动到匹配的传送节点 */
	void HandleTeleportArrival(FName NodeID);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	TSubclassOf<AAlex_PlayerCharacter> DefaultPawnClassRef;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	TSubclassOf<AAlex_PlayerController> PlayerControllerClassRef;
	
};
