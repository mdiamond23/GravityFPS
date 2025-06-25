// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "GravityFPS/GravityFPSComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"
#include "GravityFPS/GravityFPSComponents/LagCompensationComponent.h"


void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
    AWeapon::Fire(FVector());
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr) {
        return;
    }
    AController* InstigatorController = OwnerPawn->GetController();

    const USkeletalMeshSocket* MuzzleFlash = GetWeaponMesh()->GetSocketByName("Muzzle");
    if (MuzzleFlash) {
        const FTransform SocketTransform = MuzzleFlash->GetSocketTransform(GetWeaponMesh());
        const FVector Start = SocketTransform.GetLocation();

        // Maps Hit Character to number of times hit
        TMap<APlayerCharacter*, uint32> HitMap;
        for (const auto& HitTarget : HitTargets) {
            FHitResult FireHit;
            WeaponTraceHit(Start, HitTarget, FireHit);

            APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(FireHit.GetActor());
            if (PlayerCharacter) {
                if (HitMap.Contains(PlayerCharacter)) ++HitMap[PlayerCharacter];
                else HitMap.Emplace(PlayerCharacter, 1);
            }

            if (FireHit.bBlockingHit && OwnerPawn && OwnerPawn->IsLocallyControlled())
            {
                if (ImpactParticle)
                    UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactParticle, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
                if (HitSound)
                    UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
            }

            if (FireHit.bBlockingHit && HasAuthority())
            {
                MulticastImpactFX(FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
            }
        }

        float FinalDamage = CalculateFinalDamage();

        TArray<APlayerCharacter*> HitCharacters;

        for (const auto& HitPair : HitMap) {
            if (HitPair.Key && InstigatorController) {
                bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
                if (HasAuthority() && bCauseAuthDamage) {
                    UGameplayStatics::ApplyDamage(
                        HitPair.Key, // Character that was hit
                        FinalDamage * HitPair.Value, // Multiply Final Damage by number of times hit
                        InstigatorController,
                        this,
                        UDamageType::StaticClass()
                    );
                }
                HitCharacters.Add(HitPair.Key);
            }
        }
        if (!HasAuthority() && bUseServerSideRewind) {
            OwnerPlayerCharacter = !OwnerPlayerCharacter ? Cast<APlayerCharacter>(OwnerPawn) : OwnerPlayerCharacter;
            OwnerPlayerController = !OwnerPlayerController ? Cast<ACharacterPlayerController>(InstigatorController) : OwnerPlayerController;

            if (OwnerPlayerCharacter && OwnerPlayerController && OwnerPlayerCharacter->GetLagCompensation() && OwnerPlayerCharacter->IsLocallyControlled() && OwnerPlayerCharacter->IsLocallyControlled()) {
                OwnerPlayerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
                    HitCharacters,
                    Start,
                    HitTargets,
                    OwnerPlayerController->GetServerTime() - OwnerPlayerController->SingleTripTime
                );
            }
        }
    }

}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
    const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("Muzzle");
    if (!MuzzleSocket) return;

    const FTransform SocketTransfrom = MuzzleSocket->GetSocketTransform(GetWeaponMesh());
    const FVector TraceStart = SocketTransfrom.GetLocation();

    const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
    const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

    for (uint32 i = 0; i < NumPellets; ++i) {
        const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
        const FVector EndLoc = SphereCenter + RandVec;
        FVector ToEndLoc = EndLoc - TraceStart;

        ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
        HitTargets.Add(ToEndLoc);
    }
}

float AShotgun::CalculateFinalDamage()
{
    APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (!OwnerCharacter) return Damage;

    UCombatComponent* CombatComp = OwnerCharacter->GetCombat();

    return Damage * CombatComp->GetPowerMultiplier();
}
