// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GravityFPS/GravityFPSComponents/LagCompensationComponent.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed)) {
		if (ProjectileMovementComponent) {
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif



void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalInpulse, const FHitResult& Hit)
{
	APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!OwnerCharacter) {
		Super::OnHit(HitComp, OtherActor, OtherComp, NormalInpulse, Hit);
		return;
	}

	ACharacterPlayerController* OwnerController = Cast<ACharacterPlayerController>(OwnerCharacter->Controller);

	APlayerCharacter* HitCharacter = Cast<APlayerCharacter>(OtherActor);

	const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? Damage * HeadshotMultiplier : Damage;
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		ACharacterPlayerController* ShooterController = Cast<ACharacterPlayerController>(OwnerCharacter->Controller);
		APlayerCharacter* Victim = Cast<APlayerCharacter>(OtherActor);
		if (ShooterController && Victim)
		{
			bool bVictimHasShield = Victim->GetShield() > 0.f;
			ShooterController->ShowDamageMarker(DamageToCause, bVictimHasShield);
		}
	}

	// Send signal to spawn hit marker
	// Non-SSR, direct damage, only on server
	if (HasAuthority() && !bUseServerSideRewind) {
		UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
		Super::OnHit(HitComp, OtherActor, OtherComp, NormalInpulse, Hit);
		return;
	}

	// SSR branch -- only locally controlled client fires SSR request!
	else if (bUseServerSideRewind && OwnerCharacter->IsLocallyControlled() && HitCharacter && OwnerCharacter->GetLagCompensation()) {
		if (OwnerController) {
			OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
				HitCharacter,
				TraceStart,
				InitialVelocity,
				OwnerController->GetServerTime() - OwnerController->SingleTripTime
			);
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalInpulse, Hit);
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
}
