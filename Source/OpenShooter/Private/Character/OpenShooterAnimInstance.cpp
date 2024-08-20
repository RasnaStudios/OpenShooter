// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/OpenShooterAnimInstance.h"

#include "Character/OpenShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

void UOpenShooterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    OpenShooterCharacter = Cast<AOpenShooterCharacter>(TryGetPawnOwner());
}

void UOpenShooterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // We need to make sure that the character is valid
    if (OpenShooterCharacter == nullptr)
    {
        OpenShooterCharacter = Cast<AOpenShooterCharacter>(TryGetPawnOwner());
    }
    if (OpenShooterCharacter == nullptr)
        return;

    FVector Velocity = OpenShooterCharacter->GetVelocity();
    Velocity.Z = 0;    // We only want the horizontal velocity
    Speed = Velocity.Size();

    bIsInAir = OpenShooterCharacter->GetCharacterMovement()->IsFalling();
    bIsAccelerating = OpenShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0;
    bIsCrouched = OpenShooterCharacter->bIsCrouched;
    bIsAiming = OpenShooterCharacter->IsAiming();
    TurningInPlace = OpenShooterCharacter->GetTurningInPlace();

    EquippedWeapon = OpenShooterCharacter->GetEquippedWeapon();
    bWeaponEquipped = OpenShooterCharacter->IsWeaponEquipped();

    // Offset Yaw for strafing
    const FRotator AimRotation = OpenShooterCharacter->GetBaseAimRotation();
    const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(OpenShooterCharacter->GetVelocity());
    YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

    // Lean
    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = OpenShooterCharacter->GetActorRotation();
    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
    const float Target = Delta.Yaw / DeltaSeconds;
    const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.0f);
    Lean = FMath::Clamp(Interp, -90.0f, 90.0f);    // if we mouse quickly, we don't want the lean to be too much

    // Aim Offsets
    AimOffset_Yaw = OpenShooterCharacter->GetAimOffsetYaw();
    AimOffset_Pitch = OpenShooterCharacter->GetAimOffsetPitch();

    if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetMesh() && OpenShooterCharacter->GetMesh())
    {
        LeftHandTransform =
            EquippedWeapon->GetMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
        // We want the socket location to be relative to the right hand. This is because the weapon should not be asjusted or moved
        // relative to the right hand at runtime. The right hand is our reference in the bone space
        FVector LeftHandTargetLocation;
        FRotator LeftHandTargetRotation;
        OpenShooterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(),
            FRotator::ZeroRotator, LeftHandTargetLocation, LeftHandTargetRotation);
        // Now we set the location and the rotation of the left hand to the target transform
        LeftHandTransform.SetLocation(LeftHandTargetLocation);
        LeftHandTransform.SetRotation(LeftHandTargetRotation.Quaternion());

        // We want the right hand to look at the hit target, because the aim offset is not matching with the crosshair
        if (OpenShooterCharacter->IsLocallyControlled())
        {    // we don't waste computation on the server. The client will tell the server where it's shooting
            bLocallyControlled = true;
            FTransform RightHandTransform =
                EquippedWeapon->GetMesh()->GetSocketTransform(FName("RightHandSocket"), ERelativeTransformSpace::RTS_World);

            const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(),
                RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - OpenShooterCharacter->GetHitTarget()));

            // We interpolate the rotation of the right hand towards the hit target so we don't see the hand snapping to a new
            // rotation
            RightHandRotation = UKismetMathLibrary::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 10.0f);
        }

        // FOR A NEW CHARACTER: check the right hand socket and rotate it until the two debug lines are aligned
        // FTransform MuzzleTipTransform = EquippedWeapon->GetMesh()->GetSocketTransform(FName("MuzzleFlash"),
        // ERelativeTransformSpace::RTS_World); FVector
        // MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X)); DrawDebugLine(GetWorld(),
        // MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000, FColor::Red, false, 0.1f, 0, 1);
        // DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), OpenShooterCharacter->GetHitTarget(), FColor::Yellow, false,
        // 0.1f, 0, 1);
    }
}
