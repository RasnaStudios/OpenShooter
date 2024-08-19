// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

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

    void Fire(bool bButtonPressed);

    // This is necessary to replicate the sounds and visuals of the weapon firing
    UFUNCTION(Server, Reliable)
    void ServerFire(const FVector_NetQuantize& TraceHitTarget);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

    void TraceUnderCrosshair(FHitResult& HitResult);

    void SetHUDCrosshair(float DeltaSeconds);

private:
    AOpenShooterCharacter* Character;
    AOpenShooterPlayerController* Controller;
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

    // HUD and Crosshair
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
    float CrosshairVelocityFactor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
    float CrosshairInAirVelocityFactor;

    FVector HitTarget;
};
