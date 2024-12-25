// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CombatComponent.h"
#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairInterface.h"
#include "Logging/LogMacros.h"
#include "Types/TurningInPlace.h"

#include "OpenShooterCharacter.generated.h"

class AOpenShooterPlayerState;
class UTimelineComponent;
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

    UFUNCTION(NetMulticast, Unreliable)    // Easiest implementation for the client to play the impact effects
    void MulticastPlayImpactEffects(FVector_NetQuantize ImpactPoint);

    // We need to run the simulated proxies' turn in place only when necessary instead of doing in tick (because tick is not fast
    // enough)
    virtual void OnRep_ReplicatedMovement() override;

    void Eliminate();
    void PlayEliminationMontage() const;

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
    UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon, VisibleAnywhere, Category = "Weapon")
    AWeapon* OverlappingWeapon;

    UFUNCTION()
    void OnRep_OverlappingWeapon(const AWeapon* LastWeapon) const;

    // Main attributes

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Combat", meta = (AllowPrivateAccess = "true"))
    UCombatComponent* Combat;

    AOpenShooterPlayerController* PlayerController;

    // Camera Hiding

    void HideCameraIfCharacterClose() const;
    void UpdateHUDHealth();

    UPROPERTY(EditAnywhere, Category = "Components|Camera")
    float CameraHideDistanceThreshold = 200.f;

    // Player Health

    // This will be called on the clients when the Health variable is updated on the server
    UFUNCTION()
    void OnRep_Health();

    UPROPERTY(EditAnywhere, Category = "Player Stats")
    float MaxHealth = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
    float Health = MaxHealth;

    UFUNCTION()    // Bound to OnTakeAnyDamage event in BeginPlay. It is called when the ProjectileBullet calls ApplyDamage to char
    void ReceiveDamage(
        AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

    // This is necessary in the for the animation blueprint ot know when to skip the other slots (like weapon, fabrik, etc.)
    bool bEliminated = false;

    // Elimination can be done in multicast (but reliable) because it's only setting animation behaviour
    UFUNCTION(NetMulticast, Reliable)
    void MulticastEliminate();

    FTimerHandle EliminationTimer;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")    // Edit only in the default character (not in the instances)
    float EliminationDelay = 3.0f;

    void EliminationFinished();

    // Elimination Bot

    UPROPERTY(EditAnywhere, Category = "Effects")
    UParticleSystem* EliminationBotEffects;

    UPROPERTY(EditAnywhere, Category = "Effects")
    USoundCue* EliminationSound;

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

    // Poll and initialize any relevant data for the beginning of the game (HUD, etc.)
    void PollInit();

    UPROPERTY()    // set as uproperty to initialize it and avoid crashes
    AOpenShooterPlayerState* OSPlayerState;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> EliminationMontage;

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

    // Dissolve effect

    UPROPERTY(VisibleAnywhere, Category = "Combat")
    UTimelineComponent* DissolveTimeline;    // This is the component that contains the timeline

    FOnTimelineFloat DissolveTrack;    // This is the delegate that will be called every frame to update the dissolve material

    UPROPERTY(EditAnywhere, Category = "Effects")
    UCurveFloat* DissolveCurve;    // This is the curve that will be used to update the dissolve material

    // Callback function called every frame to update the dissolve material
    UFUNCTION()
    void UpdateDissolveMaterial(float DissolveValue);

    // Function to start the dissolve effect
    void StartDissolve();

    // Dynamic instance that we can change runtime, created from the mesh material
    UPROPERTY(VisibleAnywhere, Category = "Effects")
    UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

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
    FORCEINLINE bool IsEliminated() const { return bEliminated; }
    FORCEINLINE float GetHealth() const { return Health; }
    FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

    AWeapon* GetEquippedWeapon() const;
};
