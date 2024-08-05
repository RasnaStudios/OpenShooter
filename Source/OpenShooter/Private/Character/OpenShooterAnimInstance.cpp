// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Character/OpenShooterAnimInstance.h"

#include "Character/OpenShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
}
