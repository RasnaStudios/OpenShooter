// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "HUD/OpenShooterHUD.h"
#include "Weapon/WeaponTypes.h"

#include "CombatComponent.generated.h"

class AOpenShooterHUD;
class AOpenShooterPlayerController;
class AWeapon;
class AOpenShooterCharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OPENSHOOTER_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UCombatComponent();
    // The character that owns this combat component is a friend of this class
    // This is to allow the character to access the protected and private members of this class
    friend class AOpenShooterCharacter;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Equip a weapon
    void EquipWeapon(AWeapon* Weapon);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    // Set Aim
    void SetAiming(bool bIsAiming);

    // We need this RPC from the client to tell the server to set the aiming state
    // This is because the aiming state is handled on the client that pressed the button
    // and then replicated from the client to the server, and then from the server to all the clients
    // so that the aiming state is consistent across all clients and the animations are in sync
    UFUNCTION(Server, Reliable)
    void ServerSetAiming(bool bIsAiming);

    // We need this to show strafing/leaning animations on all the clients
    UFUNCTION()
    void OnRep_EquippedWeapon() const;
    void Fire();
    bool CanFire() const;

    void FireButtonPressed(bool bButtonPressed);

    // This is necessary to replicate the sounds and visuals of the weapon firing
    UFUNCTION(Server, Reliable)
    void ServerFire(const FVector_NetQuantize& TraceHitTarget);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

    void TraceUnderCrosshair(FHitResult& HitResult);

    void SetHUDCrosshair(float DeltaSeconds);

private:
    UPROPERTY()
    AOpenShooterCharacter* Character;
    UPROPERTY()
    AOpenShooterPlayerController* Controller;
    UPROPERTY()
    AOpenShooterHUD* HUD;

    UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
    AWeapon* EquippedWeapon;

    UPROPERTY(Replicated)
    bool bAiming;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float BaseWalkSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float AimWalkSpeed;

    bool bFireButtonPressed;    // we don't replicate this because we could have automatic weapons, so it would be hard
    // to replicate the changes in the button press state. We use instead multicast RPCs

    bool bCanFire = true;

    // = HUD and Crosshair =

    FHUDPackage HUDPackage;

    // Initial crosshair spread
    UPROPERTY(EditAnywhere, Category = "HUD")
    float BaselineCrosshairSpread = 0.5f;

    // Default crosshair spreading velocity
    UPROPERTY(EditAnywhere, Category = "HUD")
    float CrosshairVelocityFactor;

    // Crosshair spreading velocity when in the air
    UPROPERTY(EditAnywhere, Category = "HUD")
    float CrosshairInAirVelocityFactor;

    // Crosshair spreading when aiming
    UPROPERTY(EditAnywhere, Category = "HUD")
    float CrosshairAimFactor;

    // Crosshair spreading when shooting
    UPROPERTY(EditAnywhere, Category = "HUD")
    float CrosshairShootingFactor;

    // Crosshair spreading when shooting
    UPROPERTY(EditAnywhere, Category = "HUD")
    float CrosshairOnTargetFactor;

    // The point where the crosshair is pointing, used for correcting the right hand rotation to point towards the crosshair
    // (Done in the animation blueprint)
    FVector HitTarget;
    bool OnTarget;    // if the crosshair is on a enemy target

    // = Aim and FOV =
    UPROPERTY(EditAnywhere, Category = "Combat")
    float DefaultFOV;    // FOV when not aiming set to the camera's base FOV in BeginPlay

    float CurrentFOV;    // Current FOV of the camera

    UPROPERTY(EditAnywhere, Category = "Combat")
    float ZoomedFOV = 30.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float ZoomedInterpSpeed = 20.f;

    void InterpFOV(float DeltaSeconds);

    // = Automatic FireButtonPressed =

    FTimerHandle FireTimer;

    void StartFireTimer();       // To start the timer
    void FireTimerFinished();    // Callback called when the timer finishes

    // = Weapons =

    UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
    int32 CarriedAmmo;

    UFUNCTION()
    void OnRep_CarriedAmmo();

    UPROPERTY(EditAnywhere, Category = "Combat")
    TMap<EWeaponType, int32> CarriedAmmoMap;    // maps cannot be replicated, so we use the CarriedAmmo variable
};
