// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Character/OpenShooterPlayerController.h"
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "OpenShooterGameMode.generated.h"

class AOpenShooterCharacter;
class AOpenShooterPlayerController;

UCLASS(minimalapi)
class AOpenShooterGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    AOpenShooterGameMode();

    virtual void PlayerEliminated(AOpenShooterCharacter* EliminatedCharacter, AOpenShooterPlayerController* VictimController,
        AOpenShooterPlayerController* AttackerController);

    virtual void RequestRespawn(ACharacter* EliminatedCharacter, AOpenShooterPlayerController* PlayerController);

    UPROPERTY(EditDefaultsOnly)
    float WarmupTime = 10.f;

protected:
    virtual void BeginPlay() override;

private:
    FTimerHandle WarmupTimerHandle;    // Tracks the warmup timer before starting the match.
};
