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
    void SetHUDAnnoucement(const FText& Message, float DisplayTime = 5.0f);
    void HideAnnoucementText();

protected:
    virtual void BeginPlay() override;

private:
    FTimerHandle HideAnnoucementTextTimerHandle;
};
