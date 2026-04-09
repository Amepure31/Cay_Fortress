// Fill out your copyright notice in the Description page of Project Settings.

#include "Alex_PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AAlex_PlayerCharacter::AAlex_PlayerCharacter()
{
	// 创建弹簧臂组件
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	
	// 设置弹簧臂属性
	CameraBoom->TargetArmLength = 300.0f; // 摄像机距离角色的距离
	CameraBoom->bUsePawnControlRotation = true; // 允许弹簧臂跟随控制器旋转
	CameraBoom->bInheritPitch = false; // 不继承角色的俯仰旋转
	CameraBoom->bInheritRoll = false; // 不继承角色的翻滚旋转
	
	// 创建摄像机组件
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	
	// 设置摄像机属性
	FollowCamera->bUsePawnControlRotation = false; // 摄像机不直接跟随控制器旋转，而是通过弹簧臂
	
	// 设置角色属性
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 设置角色移动组件
	if (GetCharacterMovement())
	{
		// 设置最大加速度
		GetCharacterMovement()->MaxAcceleration = 1000.0f;
		// 设置行走时的减速度
		GetCharacterMovement()->BrakingDecelerationWalking = 400.0f;
		// 设置最大行走速度
		GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	}
}
