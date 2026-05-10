// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/AnimNotify_EnemyAttackDamage.h"
#include "Enemy/EnemyCharacter.h"

void UAnimNotify_EnemyAttackDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	AEnemyCharacter* const Enemy = Cast<AEnemyCharacter>(MeshComp->GetOwner());
	if (!Enemy)
	{
		return;
	}

	Enemy->PerformAttackDamageToPlayer();
}
