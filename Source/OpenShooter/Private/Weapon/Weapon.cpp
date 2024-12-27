// Copyright (c) 2024 Rasna Studios. All rights reserved.

#include "Weapon/Weapon.h"

#include "Character/OpenShooterCharacter.h"
#include "Character/OpenShooterPlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Casing.h"

// Sets default values
AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    // Set the collision response for the weapon mesh
    WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

    // Disable collision for the weapon mesh initially
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Create the area sphere for the weapon to collect
    AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
    AreaSphere->SetupAttachment(WeaponMesh);

    // Set the collision response for the area sphere to ignore all channels because we will handle the collision in the server
    AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
    AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

    PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
    PickupWidget->SetupAttachment(RootComponent);
    PickupWidget->SetDrawAtDesiredSize(true);
    PickupWidget->SetWidgetSpace(EWidgetSpace::Screen);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWeapon, WeaponState);
    DOREPLIFETIME(
        AWeapon, Ammo);    // We replicate the ammo variable to all clients because when the weapon is dropped, the ammo
                           // should be replicated to the client to show the correct ammo count when the weapon is picked up
}

void AWeapon::ShowPickupWidget(const bool bShow) const
{
    if (PickupWidget)
    {
        PickupWidget->SetVisibility(bShow);
    }
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
    Super::BeginPlay();

    // We enable the collision for the area sphere only on the server
    if (HasAuthority())
    {
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    }

    // Hide the pickup widget initially
    if (PickupWidget)
        PickupWidget->SetVisibility(false);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Set the overlapping weapon for the character
    if (AOpenShooterCharacter* Character = Cast<AOpenShooterCharacter>(OtherActor))
        Character->SetOverlappingWeapon(this);
}

void AWeapon::OnSphereEndOverlap(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // Set the overlapping weapon for the character
    if (AOpenShooterCharacter* Character = Cast<AOpenShooterCharacter>(OtherActor))
        Character->SetOverlappingWeapon(nullptr);
}

void AWeapon::OnRep_Owner()
{
    Super::OnRep_Owner();

    if (Owner == nullptr)
    {
        // Clear the owner and controller references when the weapon is picked up (for the server it is done in the EquipWeapon
        // function)
        OwnerCharacter = nullptr;
        OwnerController = nullptr;
    }
    else
    {
        // The pickup changes owner, so this function is called when the weapon is picked up.
        // We update the HUD with the ammo count.
        // We override this to update the ammo count on the client. For the server we call the same
        // function in the EquipWeapon function.
        SetHUDAmmo();
    }
}

void AWeapon::OnRep_Ammo()
{
    // Update the HUD with the ammo count when the ammo changes
    SetHUDAmmo();
}

void AWeapon::SpendRound()
{
    // After firing, we spend a round
    // Subtract 1 from ammo
    Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
    // Update the HUD
    SetHUDAmmo();
}

void AWeapon::Fire(const FVector& HitTarget)
{
    if (FireAnimation)
        WeaponMesh->PlayAnimation(FireAnimation, false);

    // Spawn the casing from the Ammo socket
    if (CasingClass)
    {
        const FTransform CasingTransform = WeaponMesh->GetSocketTransform(FName("AmmoEject"));

        if (UWorld* World = GetWorld())
            ACasing* Casing = World->SpawnActor<ACasing>(CasingClass, CasingTransform);
    }
    SpendRound();    // subtract 1 from ammo and update the HUD
}

// This function runs on the server does everything that needs to be done when the weapon state change, on the server.
// The client will receive the state change and will run the OnRep_WeaponState function to update the client state. (e.g. hide the
// pickup widget)
void AWeapon::SetWeaponState(const EWeaponState State)
{
    WeaponState = State;
    switch (WeaponState)
    {
        case EWeaponState::EWS_Equipped:
            ShowPickupWidget(false);
            AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            WeaponMesh->SetSimulatePhysics(false);
            WeaponMesh->SetEnableGravity(false);
            break;
        case EWeaponState::EWS_Dropped:
            if (HasAuthority())    // we use this because we call this function in the client in
                                   // CombatComponent::OnRep_EquippedWeapon
                AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            WeaponMesh->SetSimulatePhysics(true);
            WeaponMesh->SetEnableGravity(true);
            WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            break;
        default:
            break;
    }
}

void AWeapon::OnRep_WeaponState(EWeaponState PreviousState)
{
    switch (WeaponState)
    {
        case EWeaponState::EWS_Equipped:
            PickupWidget->SetVisibility(false);
            WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            WeaponMesh->SetSimulatePhysics(false);
            WeaponMesh->SetEnableGravity(false);
            break;
        case EWeaponState::EWS_Dropped:
            WeaponMesh->SetSimulatePhysics(true);
            WeaponMesh->SetEnableGravity(true);
            WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            break;
        default:
            break;
    }
}

void AWeapon::Drop()
{
    SetWeaponState(EWeaponState::EWS_Dropped);
    const FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepWorld, false);
    WeaponMesh->DetachFromComponent(DetachmentRules);
    SetOwner(nullptr);    // Remove the owner so the weapon can be picked up by other characters

    // We clear the owner and controller cached references in the server.
    // The client will receive the OnRep_Owner function and will clear the references there.
    OwnerCharacter = nullptr;
    OwnerController = nullptr;
    SetHUDAmmo();
}

void AWeapon::SetHUDAmmo()
{
    // Update the HUD with the ammo count
    // This is called when the weapon is picked up or when the ammo changes after firing
    OwnerCharacter = OwnerCharacter == nullptr ? Cast<AOpenShooterCharacter>(GetOwner()) : OwnerCharacter;
    if (OwnerCharacter)
    {
        OwnerController =
            OwnerController == nullptr ? Cast<AOpenShooterPlayerController>(OwnerCharacter->GetController()) : OwnerController;
        if (OwnerController)
            OwnerController->SetHUDWeaponAmmo(Ammo);
    }
}
