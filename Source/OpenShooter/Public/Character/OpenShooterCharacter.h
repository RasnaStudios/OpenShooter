// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CombatComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairInterface.h"
#include "Logging/LogMacros.h"
#include "Types/TurningInPlace.h"

#include "OpenShooterCharacter.generated.h"

class UCombatComponent;
class AWeapon;
class UWidgetComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class USoundCue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class AOpenShooterCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
    GENERATED_BODY()

public:
    AOpenShooterCharacter();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetOverlappingWeapon(AWeapon* Weapon);

    bool IsWeaponEquipped() const;

    bool IsAiming() const;

    void PlayFireMontage(bool bAiming) const;
    void PlayHitReactMontage() const;

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastHit(FVector_NetQuantize HitLocationNormal);

    // We need to run the simulated proxies' turn in place only when necessary instead of doing in tick (because tick is not fast
    // enough)
    virtual void OnRep_ReplicatedMovement() override;

private:
    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction;

    /** Equip Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* EquipAction;

    /** Crunch Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* CrouchAction;

    /** Aim Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* AimAction;

    /** FireButtonPressed Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* FireAction;

    // The widget component that will be displayed above the character
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|HUD", meta = (AllowPrivateAccess = "true"))
    UWidgetComponent* OverHeadWidget;

    // The weapon that the character is overlapping with
    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_OverlappingWeapon, Category = "Weapon")
    AWeapon* OverlappingWeapon;

    UFUNCTION()
    void OnRep_OverlappingWeapon(const AWeapon* LastWeapon) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Combat", meta = (AllowPrivateAccess = "true"))
    UCombatComponent* Combat;

    void HideCameraIfCharacterClose() const;

    UPROPERTY(EditAnywhere, Category = "Components|Camera")
    float CameraHideDistanceThreshold = 200.f;

    // Player Health
    UFUNCTION()
    void OnRep_Health();

    UPROPERTY(EditAnywhere, Category = "Player Stats")
    float MaxHealth = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
    float Health = MaxHealth;

protected:
    // APawn interface
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

    virtual void Jump() override;

    // Called when the Equip action is pressed
    void EquipPressed();

    // Called when the Equip action is pressed
    void CrouchPressed();

    // Remote Procedure Call sent to the server when the Equip action is pressed
    UFUNCTION(Server, Reliable)
    void ServerEquipPressed();

    // Aiming
    void AimPressed();
    void AimReleased();
    void CalculateAimOffsetPitch();
    float CalculateSpeed() const;
    void AimOffset(float DeltaSeconds);

    void SimProxiesTurn();    // Simulated proxies cannot smoothly turn in place, so we need to simulate it

    // FireButtonPressed
    void FirePressed();
    void FireReleased();

private:
    float AimOffset_Yaw;
    float AimOffset_Pitch;
    FRotator StartingAimRotation;

    ETurningInPlace TurningInPlace;
    void TurnInPlace(float DeltaSeconds);
    float InterpolatedAimOffsetYaw;    // To interpolate the aim offset yaw to 0 when turning in place

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> FireWeaponMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> HitReactMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    TObjectPtr<USoundCue> HitSound;

    UPROPERTY(EditAnywhere, Category = "Combat")
    TObjectPtr<UParticleSystem> HitParticles;

    // Variable to tell the anim blueprint that the character should rotate the root bone (when moving the mouse to a big angle)
    // This will happen only in server or autonomous proxy. Therefore, we need this to blend poses by bool
    bool bRotateRootBone = false;

    // We need these to tell the clients to play the turn in place animation (because the simulated proxies cannot turn in place)
    float TurnThreshold = 0.5f;
    FRotator ProxyRotationLastFrame;
    FRotator ProxyRotation;
    float ProxyYaw;
    float TimeSinceLastMovementReplication;

public:
    /** Returns CameraBoom subobject **/
    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    /** Returns FollowCamera subobject **/
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE float GetAimOffsetYaw() const { return AimOffset_Yaw; }
    FORCEINLINE float GetAimOffsetPitch() const { return AimOffset_Pitch; }
    FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
    FORCEINLINE FVector GetHitTarget() const { return Combat ? Combat->HitTarget : FVector(); }
    FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }

    AWeapon* GetEquippedWeapon() const;
};
