// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_Minimap.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/TextureRenderTarget2D.h"

void UUI_Minimap::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// Center player indicator
	if (PlayerIndicator)
	{
		if (UOverlaySlot* IndicatorSlot = Cast<UOverlaySlot>(PlayerIndicator->Slot))
		{
			IndicatorSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
			IndicatorSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		}
	}
}

void UUI_Minimap::NativeDestruct()
{
	EnemyMarkerPool.Empty();
	Super::NativeDestruct();
}

void UUI_Minimap::SetLargeMode(bool bLarge)
{
	bLargeMap = bLarge;
	SetRenderScale(bLarge ? FVector2D(2.5f, 2.5f) : FVector2D(1.f, 1.f));
}

void UUI_Minimap::SetRenderTarget(UTextureRenderTarget2D* RT)
{
	if (!MinimapImage || !RT) return;
	MinimapImage->SetBrushResourceObject(RT);
}

void UUI_Minimap::SetAreaName(FText Name)
{
	if (AreaNameText)
	{
		AreaNameText->SetText(Name);
	}
}

void UUI_Minimap::SetPlayerYaw(float Yaw, FVector2D PixelOffset)
{
	if (PlayerIndicator)
	{
		PlayerIndicator->SetRenderTransformAngle(Yaw - 90.f);
		PlayerIndicator->SetRenderTranslation(PixelOffset);
	}
}

void UUI_Minimap::UpdateEnemyMarkers(const TArray<FVector2D>& Offsets, const TArray<float>& Yaws)
{
	if (!EnemyOverlay) return;

	const int32 Count = FMath::Min(Offsets.Num(), Yaws.Num());
	EnsureEnemyMarkerPool(Count);

	for (int32 i = 0; i < EnemyMarkerPool.Num(); ++i)
	{
		UImage* Marker = EnemyMarkerPool[i];
		if (!Marker) continue;

		if (i < Count)
		{
			Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
			Marker->SetRenderTranslation(Offsets[i]);
			Marker->SetRenderTransformAngle(Yaws[i] - 90.f);
		}
		else
		{
			Marker->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UUI_Minimap::EnsureEnemyMarkerPool(int32 Count)
{
	if (!EnemyOverlay) return;

	// Expand pool if needed
	while (EnemyMarkerPool.Num() < Count)
	{
		UImage* Marker = NewObject<UImage>(this);
		if (!Marker) break;

		Marker->SetVisibility(ESlateVisibility::Hidden);
		if (EnemyMarkerTexture)
		{
			Marker->SetBrushFromTexture(EnemyMarkerTexture);
		}
		// Small marker: 12x12, pivot at center
		Marker->SetDesiredSizeOverride(FVector2D(12.f, 12.f));
		EnemyOverlay->AddChildToOverlay(Marker);

		if (UOverlaySlot* MarkerSlot = Cast<UOverlaySlot>(Marker->Slot))
		{
			MarkerSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
			MarkerSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		}

		EnemyMarkerPool.Add(Marker);
	}
}
