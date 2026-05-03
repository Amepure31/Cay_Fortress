#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

namespace CayFortressCombatLog
{
	/** 在 viewport 左上用青字提示（需 alex.CombatScreenSecs >0 或传入 DurationSeconds） */
	CAY_FORTRESS_API void NotifyScreen(const TCHAR* Message, float DurationSeconds = 0.f);

	constexpr const TCHAR* Prefix() { return TEXT(">>> AlexCombat | "); }
}

/** 瞄准 / 近战 / AnimInstance 诊断：Output Log 里搜 `AlexCombat` */
DECLARE_LOG_CATEGORY_EXTERN(LogAlexCombat, Display, All);
