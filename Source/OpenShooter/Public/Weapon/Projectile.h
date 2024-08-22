// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Projectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class USoundCue;

UCLASS()
class OPENSHOOTER_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    AProjectile();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    virtual void Destroyed() override;

    UFUNCTION()    // all the callbacks that we bind to overlaps and hit events must be UFUNCTIONs
    virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse, const FHitResult& Hit);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Componenets", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBoxComponent> CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Componenets", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Effects", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UParticleSystem> Tracer;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Componenets", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UParticleSystemComponent> TracerComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Effects", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UParticleSystem> ImpactParticles;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Effects", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USoundCue> ImpactSound;
};
