// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "OpenShooterPlayerState.generated.h"

class AOpenShooterPlayerController;
class AOpenShooterCharacter;
/**
 * In this class, we will add the defeats to the player state and replicate it to the client
 * We provide functions to update the score and the defeats and show the values in the HUD
 * We also cache the character and controller to avoid casting every time we update the HUD
 * We override the OnRep_Score function to update the HUD in the client
 * And we create the OnRep_Defeats function to update the HUD in the client when the defeats change
 */
UCLASS()
class OPENSHOOTER_API AOpenShooterPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
    // A reusable function to update the HUD with the latest score and/or defeats
    // This will be called many times, so we don't want to cast to the character every time
    // Therefore, we will cache the character and controller
    void UpdateHUD(const float* NewScore, const int32* NewDefeats);

    UPROPERTY()    // set as uproperty to initialize it and avoid crashes
    AOpenShooterCharacter* Character;
    UPROPERTY()    // set as uproperty to initialize it and avoid crashes
    AOpenShooterPlayerController* Controller;

    // Score is already in the base class and it's already replicated

    // We add the defeats to the player state, and we replicate it
    UPROPERTY(ReplicatedUsing = OnRep_Defeats)
    int32 Defeats;

    UPROPERTY(ReplicatedUsing = OnRep_AnnounceMessage)
    FString AnnoucementMessage;

public:
    // Client function to update the score. We override this function to update the HUD in the client (the base one is empty)
    virtual void OnRep_Score() override;

    // Server function to update the score. We create a new function to also update the HUD in the server (think of listen server)
    void AddToScore(float Amount);

    // Client function to update the defeats.
    UFUNCTION()
    void OnRep_Defeats();

    // Server function to update the score. We create a new function to also update the HUD in the server (think of listen server)
    void AddToDefeats(int32 Amount);

    // Client function to show the annoucement message
    UFUNCTION()
    void OnRep_AnnounceMessage();

    // Server function to show the annoucement message for listen-server player
    void SetAnnoucementMessage(const FString& Message);
    void ClearAnnoucementMessage();
};
