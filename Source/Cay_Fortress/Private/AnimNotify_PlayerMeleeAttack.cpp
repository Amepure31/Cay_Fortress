// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNotify_PlayerMeleeAttack.h"
#include "Alex_PlayerCharacter.h"

void UAnimNotify_PlayerMeleeAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	AAlex_PlayerCharacter* const Player = Cast<AAlex_PlayerCharacter>(MeshComp->GetOwner());
	if (!Player)
	{
		return;
	}

	Player->PerformMeleeHitDetection();
}
