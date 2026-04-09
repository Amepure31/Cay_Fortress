// Fill out your copyright notice in the Description page of Project Settings.

#include "Alex_PlayerController.h"
#include "GameFramework/Character.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

AAlex_PlayerController::AAlex_PlayerController()
{
}

void AAlex_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Get the local player subsystem
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		// Get the enhanced input subsystem
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			// Add the default mapping context
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AAlex_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Cast to enhanced input component
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Bind move action
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAlex_PlayerController::Move);
		
		// Bind look action
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAlex_PlayerController::Look);
		
		// Bind jump action
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AAlex_PlayerController::Jump);
	}
}

void AAlex_PlayerController::Move(const FInputActionValue& Value)
{
	// Get the movement vector
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Get the controlled pawn
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		// Calculate the movement direction
		FRotator Rotation = GetControlRotation();
		FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		// Get forward and right vectors
		FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement input
		ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AAlex_PlayerController::Look(const FInputActionValue& Value)
{
	// Get the look vector
	FVector2D LookVector = Value.Get<FVector2D>();

	// Add controller yaw and pitch input
	AddYawInput(LookVector.X);
	AddPitchInput(-LookVector.Y);
}

void AAlex_PlayerController::Jump()
{
	if (GetCharacter())
	{
		GetCharacter()->Jump();
	}
}
