#include "Cay_FortressLog.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogAlexCombat);

static TAutoConsoleVariable<float> CVarAlexCombatScreenSecs(
	TEXT("alex.CombatScreenSecs"),
	1.5f,
	TEXT("If >0, duplicate important AlexCombat messages on screen (seconds). Example: alex.CombatScreenSecs 2"),
	ECVF_Default);

void CayFortressCombatLog::NotifyScreen(const TCHAR* Message, float DurationSeconds)
{
	const float Cfg = CVarAlexCombatScreenSecs.GetValueOnGameThread();
	const float ShowFor = DurationSeconds > 0.f ? DurationSeconds : Cfg;
	if (ShowFor <= 0.f || !GEngine || !Message)
	{
		return;
	}
	const FString Line = FString(CayFortressCombatLog::Prefix()) + FString(Message);
	GEngine->AddOnScreenDebugMessage(
		871001,
		ShowFor,
		FColor(0, 220, 255),
		Line);
}
