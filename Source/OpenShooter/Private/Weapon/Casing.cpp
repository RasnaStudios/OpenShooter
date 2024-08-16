// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

/*
 * Casing class is used to spawn and eject the shell casing when the weapon is fired.
 * The casing is not replicated because it is a cosmetic effect.
 * The casing is ejected with a random impulse to give a realistic effect.
 * The casing is destroyed after a short delay.
 * The casing also plays a sound when it hits the ground.
 * The sound is played only once to avoid multiple sounds being played.
 */

ACasing::ACasing()
{
    PrimaryActorTick.bCanEverTick = false;

    CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
    SetRootComponent(CasingMesh);

    CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);    // ignore camera
    CasingMesh->SetSimulatePhysics(true);
    CasingMesh->SetEnableGravity(true);

    CasingMesh->SetNotifyRigidBodyCollision(true);    // Generate hit events (OnHit -> Destroy)

    ShellEjectionImpulse = 100.f;
}

void ACasing::BeginPlay()
{
    Super::BeginPlay();
    CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);

    // Generate a random vector for offset
    FVector RandomOffset = FVector(FMath::RandRange(-5.0f, 5.0f), FMath::RandRange(-5.0f, 5.0f), FMath::RandRange(-5.0f, 5.0f));

    // Normalize the random vector to ensure consistent magnitude
    RandomOffset.Normalize();

    // Apply random offset to the impulse
    const FVector ImpulseDirection =
        GetActorForwardVector() + RandomOffset * 0.1f;    // Adjust the scalar to control the offset magnitude

    // Add impulse to the casing mesh
    CasingMesh->AddImpulse(ImpulseDirection * ShellEjectionImpulse, NAME_None, true);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse, const FHitResult& Hit)
{
    if (ShellSound && !bHasPlayedSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShellSound, GetActorLocation());
        bHasPlayedSound = true;
    }

    // Destroy the casing after a short delay
    SetLifeSpan(2.f);
}
