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
        TMap<APlayerCharacter*, uint32> HeadshotHitMap;
        for (const auto& HitTarget : HitTargets) {
            FHitResult FireHit;
            WeaponTraceHit(Start, HitTarget, FireHit);

            APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(FireHit.GetActor());
            if (PlayerCharacter) {
                if (FireHit.BoneName.ToString() == FString("head")) { // Headshot
					if (HeadshotHitMap.Contains(PlayerCharacter)) ++HeadshotHitMap[PlayerCharacter];
					else HeadshotHitMap.Emplace(PlayerCharacter, 1);
                }
                else { // Bodyshot
                    if (HitMap.Contains(PlayerCharacter)) ++HitMap[PlayerCharacter];
                    else HitMap.Emplace(PlayerCharacter, 1);
                }
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
        // Maps Character Hit to total damage
        TMap<APlayerCharacter*, float> DamageMap;

        // Calculate bodyshot damage: times hit * FinalDamage - store in Damage Map
        for (const auto& HitPair : HitMap) {
            if (HitPair.Key) {
                DamageMap.Emplace(HitPair.Key, HitPair.Value * FinalDamage);
                HitCharacters.AddUnique(HitPair.Key);
            }
        }
        // Calculate headshot damage: times hit * FinalDamage * HeadshotMultiplier - Store in damage map
        for (const auto& HeadshotHitPair : HeadshotHitMap) {
            if (HeadshotHitPair.Key) {
                if (DamageMap.Contains(HeadshotHitPair.Key)) DamageMap[HeadshotHitPair.Key] += HeadshotHitPair.Value * FinalDamage * GetHeadshotMultiplier();
                else DamageMap.Emplace(HeadshotHitPair.Key, HeadshotHitPair.Value * FinalDamage * GetHeadshotMultiplier());

                HitCharacters.AddUnique(HeadshotHitPair.Key);
            }
        }

        // Show the Hitmarkers
        if (OwnerPawn && OwnerPawn->IsLocallyControlled())
        {
            ACharacterPlayerController* ShooterController = Cast<ACharacterPlayerController>(OwnerPawn->GetController());
            if (ShooterController)
            {
                for (const auto& PlayerToDamage : DamageMap)
                {
                    APlayerCharacter* Victim = PlayerToDamage.Key;
                    float DamageDealt = PlayerToDamage.Value;
                    if (Victim)
                    {
                        // Choose a hit location; for feedback, maybe just use Victim's chest location or an average of hit impact points if you track them
                        FVector HitLocation = Victim->GetActorLocation() + FVector(0, 0, 50.f); // adjust as needed
                        bool bVictimHasShield = Victim->GetShield() > 0.f;
                        ShooterController->ShowDamageMarker(DamageDealt, bVictimHasShield);
                    }
                }
            }
        }

        // Loop through damage map to get total damage and apply it accordingly
        bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
        if (HasAuthority() && bCauseAuthDamage && InstigatorController) {
            for (const auto& PlayerToDamage : DamageMap) {
                if (!PlayerToDamage.Key) continue;
                UGameplayStatics::ApplyDamage(
                    PlayerToDamage.Key, // Character that was hit
                    PlayerToDamage.Value, // Multiply Final Damage by number of times hit
                    InstigatorController,
                    this,
                    UDamageType::StaticClass()
                );
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
