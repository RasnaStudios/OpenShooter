// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "OpenShooterPlayerController.generated.h"

class AOpenShooterHUD;
/**
 *
 */
UCLASS()
class OPENSHOOTER_API AOpenShooterPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AOpenShooterHUD* HUD;

    void SetHUDHealth(float Health, float MaxHealth);
    void SetHUDScore(float Score);
    void SetHUDDefeats(int32 Defeats);

protected:
    virtual void BeginPlay() override;

private:
};
