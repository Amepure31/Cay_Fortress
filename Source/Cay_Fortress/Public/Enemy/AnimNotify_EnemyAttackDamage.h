// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyAttackDamage.generated.h"

/** 放在敌人攻击蒙太奇命中帧上，对玩家造成伤害并标记命中成功（触发攻击后咆哮）。 */
UCLASS(meta = (DisplayName = "Enemy Attack Damage"))
class CAY_FORTRESS_API UAnimNotify_EnemyAttackDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};