#pragma once

UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
    ETIP_NotTurning UMETA(DisplayName = "Not Turning"),
    ETIP_Left UMETA(DisplayName = "Left"),
    ETIP_Right UMETA(DisplayName = "Right"),
    ETIP_MAX UMETA(DisplayName = "DefaultMAX"),
};
