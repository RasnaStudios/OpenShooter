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
    UPROPERTY()
    AOpenShooterHUD* HUD;

    void SetHUDHealth(float Health, float MaxHealth);
    void SetHUDScore(float Score);
    void SetHUDDefeats(int32 Defeats);
    void SetHUDAnnoucement(const FString& Message, float DisplayTime = 5.0f);
    void ClearAnnoucementText();
    void SetHUDWeaponAmmo(int32 Ammo);
    void SetHUDCarriedAmmo(int32 Ammo);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    FTimerHandle HideAnnoucementTextTimerHandle;
};
