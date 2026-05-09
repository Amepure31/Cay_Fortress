// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UI_CharacterProperty.h"
#include "Alex_PlayerCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

void UUI_CharacterProperty::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshStatusDisplay();
}

void UUI_CharacterProperty::RefreshStatusDisplay()
{
	AAlex_PlayerCharacter* Alex = Cast<AAlex_PlayerCharacter>(GetOwningPlayerPawn());
	if (!Alex)
	{
		if (APlayerController* PC = GetOwningPlayer())
			Alex = Cast<AAlex_PlayerCharacter>(PC->GetPawn());
	}
	if (!Alex)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// --- Health ---
	if (Bar_Health)
		Bar_Health->SetPercent(Alex->GetHealthPercent());
	if (Text_Health)
		Text_Health->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Alex->GetHealth(), Alex->GetMaxHealth())));

	// --- Stamina ---
	if (Bar_Stamina)
		Bar_Stamina->SetPercent(Alex->GetStaminaPercent());
	if (Text_Stamina)
		Text_Stamina->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Alex->GetStamina(), Alex->GetMaxStamina())));

	// --- Hunger ---
	if (Bar_Hunger)
		Bar_Hunger->SetPercent(Alex->GetHungerPercent());
	if (Text_Hunger)
		Text_Hunger->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Alex->GetHunger(), Alex->GetMaxHunger())));

	// --- Hydration ---
	if (Bar_Hydration)
		Bar_Hydration->SetPercent(Alex->GetHydrationPercent());
	if (Text_Hydration)
		Text_Hydration->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Alex->GetHydration(), Alex->GetMaxHydration())));

	// --- Armor ---
	if (Text_ArmorValue)
		Text_ArmorValue->SetText(FText::FromString(FString::Printf(TEXT("护甲: %.0f"), Alex->GetTotalArmorValue())));
	if (Text_DamageReduction)
	{
		const float TotalPct = FMath::Min((Alex->GetTotalArmorValue() / (Alex->GetTotalArmorValue() + 100.0f)) + Alex->GetTotalDamageReduction(), 0.9f) * 100.0f;
		Text_DamageReduction->SetText(FText::FromString(FString::Printf(TEXT("减伤: %.0f%%"), TotalPct)));
	}

	// --- Carry Weight ---
	const float Carried = Alex->GetCurrentCarriedWeight();
	const float MaxCarry = Alex->GetMaxCarryWeight();
	const float WeightPct = (MaxCarry > KINDA_SMALL_NUMBER) ? FMath::Clamp(Carried / MaxCarry, 0.0f, 1.0f) : 0.0f;
	if (Bar_CarryWeight)
		Bar_CarryWeight->SetPercent(WeightPct);
	if (Text_CarryWeight)
		Text_CarryWeight->SetText(FText::FromString(FString::Printf(TEXT("%.1f / %.1f"), Carried, MaxCarry)));
}
