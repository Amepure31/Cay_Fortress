#include "UI/UI_AimPoint.h"
#include "Alex_PlayerCharacter.h"
#include "Engine/HitResult.h"
#include "Inventory/InventoryComponent.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UUI_AimPoint::NativeConstruct()
{
	Super::NativeConstruct();
	bFollowImpactPoint = true;
	bHideFollowWhenNotOnScreen = true;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UUI_AimPoint::RefreshAimDisplay()
{
	AAlex_PlayerCharacter* Alex = Cast<AAlex_PlayerCharacter>(GetOwningPlayerPawn());
	APlayerController* PC = GetOwningPlayer();
	if (!IsInViewport() || !Alex || !PC || !Alex->IsAiming())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (Alex->GetInventory() && Alex->GetInventory()->IsInventoryOpen())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (PC->bShowMouseCursor)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (ReticleCenter)
	{
		ReticleCenter->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (!bFollowImpactPoint || !ReticleFollow)
	{
		if (ReticleFollow)
		{
			ReticleFollow->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	FHitResult Hit;
	FVector TraceEnd;
	float MaxCm = 0.f;
	Alex->GetAimUiLineTrace(Hit, TraceEnd, MaxCm);
	const FVector WorldAim = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;

	FVector2D ScreenPos;
	const bool bProjected = UGameplayStatics::ProjectWorldToScreen(PC, WorldAim, ScreenPos, true);
	if (!bProjected)
	{
		ReticleFollow->SetVisibility(
			bHideFollowWhenNotOnScreen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		if (!bHideFollowWhenNotOnScreen)
		{
			ReticleFollow->SetRenderTranslation(FVector2D::ZeroVector);
		}
		return;
	}

	int32 VX = 0;
	int32 VY = 0;
	PC->GetViewportSize(VX, VY);
	if (VX <= 0 || VY <= 0)
	{
		ReticleFollow->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (bHideFollowWhenNotOnScreen)
	{
		if (ScreenPos.X < 0.f || ScreenPos.Y < 0.f || ScreenPos.X > static_cast<float>(VX)
			|| ScreenPos.Y > static_cast<float>(VY))
		{
			ReticleFollow->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
	}

	const FVector2D ViewportCenter(VX * 0.5f, VY * 0.5f);
	ReticleFollow->SetVisibility(ESlateVisibility::HitTestInvisible);
	ReticleFollow->SetRenderTranslation(ScreenPos - ViewportCenter);
}
