// Fill out your copyright notice in the Description page of Project Settings.

#include "Alex_PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Inventory/InventoryComponent.h"
#include "Equipment/EquipmentComponent.h"

AAlex_PlayerCharacter::AAlex_PlayerCharacter()
{
	// 创建背包组件
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	// 创建装备组件
	Equipment = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));

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

	// 初始化角色属性
	Health = 100.0f;
	MaxHealth = 100.0f;
	Stamina = 100.0f;
	MaxStamina = 100.0f;
	MoveSpeed = 350.0f;
	RunSpeed = 600.0f;
	SpeedTransitionTime = 0.3f;
	TargetMoveSpeed = MoveSpeed;
	CurrentMoveSpeed = MoveSpeed;
	CurrentRunSpeed = RunSpeed;

	// 设置角色移动组件
	if (GetCharacterMovement())
	{
		// 设置最大加速度
		GetCharacterMovement()->MaxAcceleration = 1000.0f;
		// 设置行走时的减速度
		GetCharacterMovement()->BrakingDecelerationWalking = 400.0f;
		// 设置最大行走速度
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
		// 设置最大跑步速度
		GetCharacterMovement()->MaxFlySpeed = RunSpeed;
	}
}

void AAlex_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

float AAlex_PlayerCharacter::GetHealth() const
{
	return Health;
}

float AAlex_PlayerCharacter::GetMaxHealth() const
{
	return MaxHealth;
}

float AAlex_PlayerCharacter::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? (Health / MaxHealth) : 0.0f;
}

void AAlex_PlayerCharacter::SetHealth(float InHealth)
{
	Health = FMath::Clamp(InHealth, 0.0f, MaxHealth);
}

void AAlex_PlayerCharacter::Heal(float Amount)
{
	if (Amount > 0.0f)
	{
		Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	}
}

float AAlex_PlayerCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Damage > 0.0f)
	{
		float FinalDamage = Damage;
		if (Equipment)
		{
			const float Reduction = Equipment->GetTotalDamageReduction();
			FinalDamage = Damage * (1.0f - Reduction);
		}
		Health = FMath::Clamp(Health - FinalDamage, 0.0f, MaxHealth);
	}
	return Damage;
}

float AAlex_PlayerCharacter::GetStamina() const
{
	return Stamina;
}

float AAlex_PlayerCharacter::GetMaxStamina() const
{
	return MaxStamina;
}

float AAlex_PlayerCharacter::GetStaminaPercent() const
{
	return MaxStamina > 0.0f ? (Stamina / MaxStamina) : 0.0f;
}

void AAlex_PlayerCharacter::SetStamina(float InStamina)
{
	Stamina = FMath::Clamp(InStamina, 0.0f, MaxStamina);
}

void AAlex_PlayerCharacter::RecoverStamina(float Amount)
{
	if (Amount > 0.0f)
	{
		Stamina = FMath::Clamp(Stamina + Amount, 0.0f, MaxStamina);
	}
}

bool AAlex_PlayerCharacter::ConsumeStamina(float Amount)
{
	if (Amount > 0.0f && Stamina >= Amount)
	{
		Stamina -= Amount;
		return true;
	}
	return false;
}

float AAlex_PlayerCharacter::GetMoveSpeed() const
{
	return MoveSpeed;
}

void AAlex_PlayerCharacter::SetMoveSpeed(float InMoveSpeed)
{
	MoveSpeed = FMath::Clamp(InMoveSpeed, 0.0f, 600.0f);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}
}

void AAlex_PlayerCharacter::SetTargetMoveSpeed(float InTargetMoveSpeed)
{
	TargetMoveSpeed = FMath::Clamp(InTargetMoveSpeed, 0.0f, 600.0f);
}

float AAlex_PlayerCharacter::GetTargetMoveSpeed() const
{
	return TargetMoveSpeed;
}

float AAlex_PlayerCharacter::GetCurrentMoveSpeed() const
{
	return CurrentMoveSpeed;
}

void AAlex_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SpeedTransitionTime > 0.0f)
	{
		CurrentMoveSpeed = FMath::FInterpTo(CurrentMoveSpeed, TargetMoveSpeed, DeltaTime, 1.0f / SpeedTransitionTime);
	}
	else
	{
		CurrentMoveSpeed = TargetMoveSpeed;
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = CurrentMoveSpeed;
	}
}

float AAlex_PlayerCharacter::GetRunSpeed() const
{
	return RunSpeed;
}

void AAlex_PlayerCharacter::SetRunSpeed(float InRunSpeed)
{
	RunSpeed = FMath::Clamp(InRunSpeed, 0.0f, 600.0f);
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxFlySpeed = RunSpeed;
	}
}
