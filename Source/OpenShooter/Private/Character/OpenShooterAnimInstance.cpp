// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/OpenShooterAnimInstance.h"

#include "Character/OpenShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
    bWeaponEquipped = OpenShooterCharacter->IsWeaponEquipped();
    bIsCrouched = OpenShooterCharacter->bIsCrouched;
    bIsAiming = OpenShooterCharacter->IsAiming();

    // Offset Yaw for strafing
    const FRotator AimRotation = OpenShooterCharacter->GetBaseAimRotation();
    const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(OpenShooterCharacter->GetVelocity());
    const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, MovementRotation);
    // In the corner case of fast movement between -180 and 180 degrees, we need to adjust the value by interpolating
    InterpolatedDeltaRotation = UKismetMathLibrary::RInterpTo(DeltaRotation, DeltaRotation, DeltaSeconds, 6.0f);

    // Lean
    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = OpenShooterCharacter->GetActorRotation();
    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
    const float Target = Delta.Yaw / DeltaSeconds;
    const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.0f);
    Lean = FMath::Clamp(Interp, -90.0f, 90.0f);    // if we mouse quickly, we don't want the lean to be too much
}
