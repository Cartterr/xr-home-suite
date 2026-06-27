#include "XRHSValidationGameMode.h"

#include "XRHSValidationPawn.h"

AXRHSValidationGameMode::AXRHSValidationGameMode()
{
	DefaultPawnClass = AXRHSValidationPawn::StaticClass();
}
