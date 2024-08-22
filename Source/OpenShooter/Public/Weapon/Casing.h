// Copyright (c) 2024 Rasna Studios. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Casing.generated.h"

class USoundCue;

UCLASS()
class OPENSHOOTER_API ACasing : public AActor
{
    GENERATED_BODY()

public:
    ACasing();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()    // all the callbacks that we bind to overlaps and hit events must be UFUNCTIONs
    virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse, const FHitResult& Hit);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componenets", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* CasingMesh;

    UPROPERTY(EditAnywhere, Category = "Casing Properties")
    float ShellEjectionImpulse;

    UPROPERTY(EditAnywhere, Category = "Casing Properties")
    TObjectPtr<USoundCue> ShellSound;

    bool bHasPlayedSound = false;

public:
    FORCEINLINE UStaticMesh* GetMesh() const { return CasingMesh->GetStaticMesh(); }
};
