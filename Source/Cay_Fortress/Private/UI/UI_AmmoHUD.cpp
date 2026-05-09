// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_AmmoHUD.h"
#include "Alex_PlayerCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"

void UUI_AmmoHUD::ApplyWeaponTypeIconWithFixedHeight(UTexture2D* Icon)
{
	if (!Image_WeaponType || !Icon)
	{
		return;
	}

	const float TexW = static_cast<float>(FMath::Max(1, Icon->GetSizeX()));
	const float TexH = static_cast<float>(FMath::Max(1, Icon->GetSizeY()));
	const float FixedH = FMath::Max(1.f, WeaponTypeIconFixedHeight);
	const float W = FixedH * (TexW / TexH);

	FSlateBrush Brush;
	Brush.SetResourceObject(Icon);
	Brush.ImageSize = FVector2D(W, FixedH);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Image_WeaponType->SetBrush(Brush);
}

void UUI_AmmoHUD::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshAmmoDisplay();
}

void UUI_AmmoHUD::RefreshAmmoDisplay()
{
	AAlex_PlayerCharacter* Alex = Cast<AAlex_PlayerCharacter>(GetOwningPlayerPawn());
	if (!Alex)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			Alex = Cast<AAlex_PlayerCharacter>(PC->GetPawn());
		}
	}
	if (!Alex)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	int32 Magazine = 0;
	int32 Reserve = 0;
	bool bPistolIcon = false;
	bool bWeapon = false;
	Alex->GetAmmoHudDisplayValues(Magazine, Reserve, bPistolIcon, bWeapon);

	UInventoryComponent* Inv = Alex->GetInventory();
	const bool bInventoryOpen = Inv && Inv->IsInventoryOpen();
	const bool bShouldShow = bWeapon && (Alex->IsAiming() || bInventoryOpen);

	if (bWeapon)
	{
		const FText MagText = FText::AsNumber(Magazine);
		const FText ResText = FText::AsNumber(Reserve);
		if (Text_AmmoCurrent)
		{
			Text_AmmoCurrent->SetText(MagText);
		}
		if (Text_AmmoReserve)
		{
			Text_AmmoReserve->SetText(ResText);
		}
	}

	if (!bShouldShow)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (Image_WeaponType)
	{
		UTexture2D* const Icon = bPistolIcon ? PistolTypeHudTexture.Get() : RifleTypeHudTexture.Get();
		if (Icon)
		{
			ApplyWeaponTypeIconWithFixedHeight(Icon);
			Image_WeaponType->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_WeaponType->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
