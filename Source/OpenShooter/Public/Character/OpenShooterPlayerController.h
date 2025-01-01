// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "OpenShooterPlayerController.generated.h"

enum class EWeaponType : uint8;
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
    void SetHUDMatchCountdown(float Countdown);
    void SetHUDScore(float Score);
    void SetHUDDefeats(int32 Defeats);
    void SetHUDAnnoucement(const FString& Message, float DisplayTime = 5.0f);
    void ClearAnnoucementText();
    void SetHUDWeaponAmmo(int32 Ammo);
    void SetHUDWeaponType(EWeaponType WeaponType);
    void SetHUDCarriedAmmo(int32 Ammo);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

    virtual void Tick(float DeltaSeconds) override;
    void SetHUDTime();

private:
    FTimerHandle HideAnnoucementTextTimerHandle;

    // Just temp, we need to move this to gamemode
    UPROPERTY(EditAnywhere)
    float MatchTime = 10.f;
    uint32 CountDownInt = 0;
};
