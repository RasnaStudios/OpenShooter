// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "OpenShooterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
    GENERATED_BODY()
public:
    UTexture2D* CrosshairsCenter;
    UTexture2D* CrosshairsLeft;
    UTexture2D* CrosshairsRight;
    UTexture2D* CrosshairsTop;
    UTexture2D* CrosshairsBottom;
    float CrosshairSpread;
};

UCLASS()
class OPENSHOOTER_API AOpenShooterHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    FHUDPackage HUDPackage;

    void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (AllowPrivateAccess = "true"))
    float CrosshairSpreadMax = 20.f;

public:
    FORCEINLINE FHUDPackage SetHUDPackage(const FHUDPackage& NewHUDPackage) { return HUDPackage = NewHUDPackage; }
};
