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

    virtual float GetServerTime();             // Synced with server world clock.
    virtual void ReceivedPlayer() override;    // Earliest point where we can sync server clock with client clock

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

    virtual void Tick(float DeltaSeconds) override;
    void SetHUDTime();

    // SYNC between client and server
    // RPCs to send the client current time to the server
    // The server is not going to use it, but it will send the current time back to the client
    // so that the client can calculate the round trip time

    // Requests the current server time, passing in the client's time when the request was sent
    UFUNCTION(Server, Reliable)
    void ServerSequestServerTime(float TimeOfClientRequest);

    // Reports the server time back to the client in response to the request to ServerRequestServerTime
    UFUNCTION(Client, Reliable)
    void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

    // Delta time between the client and the server
    float ClientServerDelta = 0.f;

    UPROPERTY(EditAnywhere, Category = Time)
    float SyncFrequencySeconds = 5.f;    // How often to sync the client and server clocks

    float SyncRunningTimeSeconds = 0.f;    // How much time is passed since last sync (used in Tick)

    void CheckTimeSync(float DeltaSeconds);    // Check if we need to sync the client and server clocks

private:
    FTimerHandle HideAnnoucementTextTimerHandle;

    // Just temp, we need to move this to gamemode
    UPROPERTY(EditAnywhere)
    float MatchTime = 10.f;
    uint32 CountDownInt = 0;
};
