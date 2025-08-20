// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponTypes.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundCue.h"
#include "GravityFPS/GravityFPSComponents/LagCompensationComponent.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"
#include "GravityFPS/GravityFPSComponents/CombatComponent.h"


void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	AController* InstigatorController = OwnerPawn->GetInstigatorController();


	const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("Muzzle");
	if (MuzzleSocket) {

		FTransform SocketTransfrom = MuzzleSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransfrom.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
		float FinalDamage = Damage; // The weapon's base damage

		if (OwnerCharacter)
		{
			UCombatComponent* CombatComp = OwnerCharacter->GetCombat();
			if (CombatComp)
			{
				FinalDamage *= CombatComp->GetPowerMultiplier();
			}
		}

		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(FireHit.GetActor());

		if (PlayerCharacter && InstigatorController) {
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			FinalDamage = FireHit.BoneName.ToString() == FString("head") ? FinalDamage * GetHeadshotMultiplier() : FinalDamage;

			if (OwnerPawn && OwnerPawn->IsLocallyControlled())
			{
				ACharacterPlayerController* ShooterController = Cast<ACharacterPlayerController>(OwnerPawn->GetController());
				if (ShooterController)
				{
					bool bVictimHasShield = PlayerCharacter->GetShield() > 0.f;
					ShooterController->ShowDamageMarker(FinalDamage, bVictimHasShield);
				}
			}
			// Send signal to spawn damage marker

			if (HasAuthority() && bCauseAuthDamage) {

				UGameplayStatics::ApplyDamage(
					PlayerCharacter,
					FinalDamage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}

			if (!HasAuthority() && bUseServerSideRewind) {
				OwnerPlayerCharacter = !OwnerPlayerCharacter ? Cast<APlayerCharacter>(OwnerPawn) : OwnerPlayerCharacter;
				OwnerPlayerController = !OwnerPlayerController ? Cast<ACharacterPlayerController>(InstigatorController) : OwnerPlayerController;

				if (OwnerPlayerCharacter && OwnerPlayerController && OwnerPlayerCharacter->GetLagCompensation() && OwnerPlayerCharacter->IsLocallyControlled()) {
					OwnerPlayerCharacter->GetLagCompensation()->ServerScoreRequest(
						PlayerCharacter,
						Start,
						HitTarget,
						OwnerPlayerController->GetServerTime() - OwnerPlayerController->SingleTripTime
					);
				}
			}
		}

		if (FireHit.bBlockingHit && HasAuthority() && InstigatorController)
		{
			MulticastImpactFX(FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
		}

		if (FireHit.bBlockingHit && OwnerPawn && OwnerPawn->IsLocallyControlled())
		{
			// Local instant feedback (not replicated, not from server)
			if (ImpactParticle)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					ImpactParticle,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint
				);
			}
		}
	}

}

void AHitScanWeapon::MulticastImpactFX_Implementation(const FVector& ImpactPoint, const FRotator& ImpactRotation)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled() && !HasAuthority()) return;

	if (ImpactParticle)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactParticle,
			ImpactPoint,
			ImpactRotation
		);
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint
		);
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();

	if (World) {
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(GetOwner());


		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility,
			TraceParams
		);
		
		FVector BeamEnd = End;

		if (OutHit.bBlockingHit) {
			BeamEnd = OutHit.ImpactPoint;
		}
		else {
			OutHit.ImpactPoint = End;
		}
		if (BeamParticles) {
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);

			if (Beam) Beam->SetVectorParameter(FName("Target"), BeamEnd);
		}
	}
}
