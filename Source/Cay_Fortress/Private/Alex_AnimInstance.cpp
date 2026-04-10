// Fill out your copyright notice in the Description page of Project Settings.


#include "Alex_AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Alex_PlayerCharacter.h"
#include "Logging/LogMacros.h"

// 动画更新事件（每帧调用）
void UAlex_AnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);


	// 获取角色
	ACharacter* Character = GetOwningCharacter();
	if (Character)
	{
		// 获取角色移动组件
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		if (Movement)
		{
			// 更新速度向量
			Velocity = Movement->Velocity;

			// 更新地面速度（不计算Z方向）
			GroundSpeed = FMath::Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);

			// 更新移动方向
			FRotator ActorRotation = Character->GetActorRotation();
			FRotator YawRotation(0.f, ActorRotation.Yaw, 0.f);

			if (Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				// 如果有移动，计算速度方向
				FVector VelocityDirection = Velocity.GetSafeNormal();
				VelocityDirection.Z = 0.0f;
				VelocityDirection.Normalize();

				// 获取角色面向方向
				FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

				// 计算速度方向与角色面向方向的夹角
				float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ForwardDirection, VelocityDirection)));

				// 限制在正负45°范围内
				if (FMath::Abs(Angle) <= 45.0f)
				{
					// 如果在45°范围内，使用速度方向
					Direction = Angle;
				}
				else
				{
					// 如果超过45°，使用角色面向方向
					Direction = 0.0f;
				}
			}
			else
			{
				// 如果没有移动，返回0
				Direction = 0.0f;
			}

			// 更新 ShouldMove：加速度不为0且地速大于0.01
			ShouldMove = Movement->GetCurrentAcceleration().SizeSquared() > KINDA_SMALL_NUMBER && GroundSpeed > 0.01f;

			// 更新 IsFalling：参考蓝图中的 IsFalling 函数
			IsFalling = Movement->IsFalling();

			// 更新跑步状态
			IsRunning = CheckIsRunning();

			// 更新移动速度和跑步速度
			if (AAlex_PlayerCharacter* PlayerCharacter = Cast<AAlex_PlayerCharacter>(Character))
			{
				MoveSpeed = PlayerCharacter->GetMoveSpeed();
				RunSpeed = PlayerCharacter->GetRunSpeed();
				CurrentMoveSpeed = PlayerCharacter->GetCurrentMoveSpeed();
			}
			else
			{
				MoveSpeed = 0.0f;
				RunSpeed = 0.0f;
				CurrentMoveSpeed = 0.0f;
			}
		}
		else
		{
			// 如果没有移动组件，重置变量
			Velocity = FVector::ZeroVector;
			GroundSpeed = 0.0f;
			Direction = 0.0f;
			ShouldMove = false;
			IsFalling = false;
			IsRunning = false;
			MoveSpeed = 0.0f;
			RunSpeed = 0.0f;
			CurrentMoveSpeed = 0.0f;
		}
	}
	else
	{
		// 如果没有角色，重置变量
		Velocity = FVector::ZeroVector;
		GroundSpeed = 0.0f;
		Direction = 0.0f;
		ShouldMove = false;
		IsFalling = false;
		IsRunning = false;
		MoveSpeed = 0.0f;
		RunSpeed = 0.0f;
		CurrentMoveSpeed = 0.0f;
	}
}

// 获取角色
ACharacter* UAlex_AnimInstance::GetOwningCharacter() const
{
	// 获取拥有者（在动画实例中，应该使用 GetOwningActor() 获取绑定的 Actor）
	AActor* Owner = GetOwningActor();
	if (Owner)
	{
		return Cast<ACharacter>(Owner);
	}
	return nullptr;
}

// 获取角色移动组件
UCharacterMovementComponent* UAlex_AnimInstance::GetCharacterMovement() const
{
	// 获取角色
	ACharacter* Character = GetOwningCharacter();
	if (Character)
	{
		return Character->GetCharacterMovement();
	}
	return nullptr;
}

// 获取玩家控制器
APlayerController* UAlex_AnimInstance::GetPlayerController() const
{
	// 获取角色
	ACharacter* Character = GetOwningCharacter();
	if (Character)
	{
		return Character->GetController<APlayerController>();
	}
	return nullptr;
}

// 检查角色是否正在跑步
bool UAlex_AnimInstance::CheckIsRunning() const
{
	// 获取角色
	ACharacter* Character = GetOwningCharacter();
	if (Character)
	{
		UCharacterMovementComponent* CharMovement = Character->GetCharacterMovement();
		if (CharMovement)
		{
			// 检查是否在移动
			if (GroundSpeed > 0.1f)
			{
				// 获取角色速度阈值
				float CurrentMaxSpeed = CharMovement->MaxWalkSpeed;
				
				// 如果当前速度超过最大行走速度，认为是跑步
				return GroundSpeed > CurrentMaxSpeed;
			}
		}
	}
	return false;
}

// Getter 函数
float UAlex_AnimInstance::GetGroundSpeed_Bp() const
{
	return GroundSpeed;
}

FVector UAlex_AnimInstance::GetVelocity_Bp() const
{
	return Velocity;
}

float UAlex_AnimInstance::GetDirection_Bp() const
{
	return Direction;
}

bool UAlex_AnimInstance::GetShouldMove_Bp() const
{
	return ShouldMove;
}

bool UAlex_AnimInstance::GetIsFalling_Bp() const
{
	return IsFalling;
}

bool UAlex_AnimInstance::GetIsRunning_Bp() const
{
	return IsRunning;
}

float UAlex_AnimInstance::GetMoveSpeed_Bp() const
{
	return MoveSpeed;
}

float UAlex_AnimInstance::GetRunSpeed_Bp() const
{
	return RunSpeed;
}

float UAlex_AnimInstance::GetCurrentMoveSpeed_Bp() const
{
	return CurrentMoveSpeed;
}
