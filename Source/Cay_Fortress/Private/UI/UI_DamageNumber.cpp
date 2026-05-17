#include "UI/UI_DamageNumber.h"
#include "Components/TextBlock.h"

void UUI_DamageNumber::SetDamage(float Damage, bool bHeadshot)
{
	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
		DamageText->SetColorAndOpacity(bHeadshot
			? FSlateColor(FLinearColor(1.0f, 0.15f, 0.1f, 1.0f))
			: FSlateColor(FLinearColor(1.0f, 0.95f, 0.3f, 1.0f)));
	}
}

void UUI_DamageNumber::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Elapsed += InDeltaTime;

	FloatOffset += FloatSpeed * InDeltaTime;
	SetRenderTranslation(FVector2D(0.0f, -FloatOffset));

	const float Alpha = 1.0f - (Elapsed / Lifetime);
	SetRenderOpacity(Alpha);

	if (Elapsed >= Lifetime)
	{
		RemoveFromParent();
	}
}
