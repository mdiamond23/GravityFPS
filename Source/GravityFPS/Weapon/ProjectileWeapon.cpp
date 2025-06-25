// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GravityFPS/GravityFPSComponents/CombatComponent.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName(FName("Muzzle"));
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();

	if (MuzzleSocket && World) {
		FTransform SocketTransform = MuzzleSocket->GetSocketTransform(GetWeaponMesh());
		// From muzzle to target from trace under crosshairs
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = InstigatorPawn;
		AProjectile* Projectile = nullptr;

		float DamageMultiplier = 1.f;
		APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
		if (OwnerCharacter)
		{
			UCombatComponent* CombatComp = OwnerCharacter->GetCombat();
			if (CombatComp)
			{
				DamageMultiplier *= CombatComp->GetPowerMultiplier();
			}
		}

		if (bUseServerSideRewind) {
			if (InstigatorPawn->HasAuthority()) { // Server
				if (InstigatorPawn->IsLocallyControlled()) { // Server host, use replicated projectile
					Projectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, Params);
					Projectile->bUseServerSideRewind = false;
					Projectile->Damage = Damage * DamageMultiplier;
				}
				else { // Server, not locally controlled, spawn non-replicated projectile without SSR
					Projectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, Params);
					Projectile->bUseServerSideRewind = true;
				}
			}
			else { // Client, using SSR
				if (InstigatorPawn->IsLocallyControlled()) { // Client, locally controlled, spawn non-replicated projectile with SSR
					Projectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, Params);
					Projectile->bUseServerSideRewind = true;
					Projectile->TraceStart = SocketTransform.GetLocation();
					Projectile->InitialVelocity = Projectile->GetActorForwardVector() * Projectile->InitialSpeed;
					Projectile->Damage = Damage * DamageMultiplier;
				}
				else { // Client, not locally controlled, spawn non-replicated projectile without SSR
					Projectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, Params);
					Projectile->bUseServerSideRewind = false;
				}
			}
		}
		else { // Weapon not using SSR
			if (InstigatorPawn->HasAuthority()) {
				Projectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, Params);
				Projectile->bUseServerSideRewind = false;
				Projectile->Damage = Damage * DamageMultiplier;
			}
		}
	}
}
