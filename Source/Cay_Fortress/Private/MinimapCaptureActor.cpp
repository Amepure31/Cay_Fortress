// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapCaptureActor.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
AMinimapCaptureActor::AMinimapCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(Root);
	SceneCapture->SetRelativeLocation(FVector(0.f, 0.f, CaptureHeight));
	SceneCapture->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCapture->OrthoWidth = OrthoWidth;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	// Minimap look: unlit, no characters, no shadows
	SceneCapture->ShowFlags.SetLighting(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetDynamicShadows(false);
	SceneCapture->ShowFlags.SetSkeletalMeshes(false);
	SceneCapture->ShowFlags.SetAmbientOcclusion(false);
	SceneCapture->ShowFlags.SetVolumetricFog(false);
}

void AMinimapCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	SceneCapture->OrthoWidth = OrthoWidth;
	SceneCapture->SetRelativeLocation(FVector(0.f, 0.f, CaptureHeight));
	if (MinimapRenderTarget)
	{
		SceneCapture->TextureTarget = MinimapRenderTarget;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (APawn* PlayerPawn = PC->GetPawn())
		{
			FollowTarget = PlayerPawn;
			// Hide player from capture
			SceneCapture->HiddenActors.AddUnique(PlayerPawn);
			// Also hide the player controller (for any attached visuals)
			SceneCapture->HiddenActors.AddUnique(PC);

			const FVector PlayerLoc = PlayerPawn->GetActorLocation();
			SetActorLocation(FVector(PlayerLoc.X, PlayerLoc.Y, CaptureHeight));
		}
	}

	// Capture at ~10 Hz
	GetWorld()->GetTimerManager().SetTimer(
		CaptureTimerHandle,
		FTimerDelegate::CreateUObject(SceneCapture, &USceneCaptureComponent2D::CaptureScene),
		0.0167f, true);

}

void AMinimapCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FollowTarget.IsValid())
	{
		return;
	}

	const FVector TargetLoc = FollowTarget->GetActorLocation();
	const FVector TargetXY(TargetLoc.X, TargetLoc.Y, CaptureHeight);

	if (FollowSpeed <= 0.f)
	{
		SetActorLocation(TargetXY);
	}
	else
	{
		const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetXY, DeltaSeconds, FollowSpeed);
		SetActorLocation(NewLoc);
	}
}

