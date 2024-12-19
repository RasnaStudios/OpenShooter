// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "OpenShooterPlayerState.generated.h"

class AOpenShooterPlayerController;
class AOpenShooterCharacter;
/**
 *
 */
UCLASS()
class OPENSHOOTER_API AOpenShooterPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // Client function to update the score
    virtual void OnRep_Score() override;

    // Server function to update the score
    void AddToScore(float Amount);

private:
    AOpenShooterCharacter* Character;
    AOpenShooterPlayerController* Controller;
};
