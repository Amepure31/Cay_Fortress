#include "Cay_FortressLog.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogAlexCombat);

static constexpr float AlexCombatScreenSecsDefault = 1.5f;

static TAutoConsoleVariable<float> CVarAlexCombatScreenSecs(
	TEXT("alex.CombatScreenSecs"),
	AlexCombatScreenSecsDefault,
	TEXT("If >0, duplicate important AlexCombat messages on screen (seconds). Example: alex.CombatScreenSecs 2"),
	ECVF_Default);

void CayFortressCombatLog::NotifyScreen(const TCHAR* Message, float DurationSeconds)
{
	if (!Message)
	{
		return;
	}

	UEngine* Engine = GEngine;
	if (!IsValid(Engine))
	{
		return;
	}

	float Cfg = AlexCombatScreenSecsDefault;
	if (IConsoleVariable* CV = IConsoleManager::Get().FindConsoleVariable(TEXT("alex.CombatScreenSecs")))
	{
		Cfg = CV->GetFloat();
	}

	const float ShowFor = DurationSeconds > 0.f ? DurationSeconds : Cfg;
	if (ShowFor <= 0.f)
	{
		return;
	}

	const FString Line = FString(CayFortressCombatLog::Prefix()) + FString(Message);
	Engine->AddOnScreenDebugMessage(
		871001,
		ShowFor,
		FColor(0, 220, 255),
		Line);
}
