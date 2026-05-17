// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayerMeleeAttack.generated.h"

/** 放在玩家近战攻击蒙太奇命中帧上，对前方敌人进行球形扫掠并造成伤害。 */
UCLASS(meta = (DisplayName = "Player Melee Attack"))
class CAY_FORTRESS_API UAnimNotify_PlayerMeleeAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
