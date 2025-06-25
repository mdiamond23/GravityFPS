// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetIsReplicated(true);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	if (ProjectileLoop)
	{
		// Optional: Check for attenuation if you want
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			false,         // bStopWhenAttachedToDestroyed
			1.f,           // Volume
			1.f,           // Pitch
			0.f           // StartTime
		);
	}
}

#if WITH_EDITOR
void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, InitialSpeed)) {
		if (ProjectileMovementComponent) {
			RocketMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileRocket::Destroyed()
{

}

void AProjectileRocket::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectileRocket, RocketMesh);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalInpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return;

	APawn* FiringPawn = GetInstigator();

	if (FiringPawn && HasAuthority()) {
		AController* FiringController = FiringPawn->GetController();
		if (FiringController) {
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World context object
				Damage, // BaseDamage
				10.f, // MinDamage
				GetActorLocation(), // Origin
				InnerRadius, // Damage Inner Radius
				OuterRadius, // Damage Outer Radius
				1.f, // DamageFalloff exponent
				UDamageType::StaticClass(), // DamageType
				TArray<AActor*>(), // IgnoreActors
				this, // DamageCauser
				FiringController // InstigatorController
			);
		}
	}

	MulticastImpactFX(GetActorLocation(), GetActorRotation());
	GetWorldTimerManager().SetTimer(
		DestroyTimer,                          // Timer handle
		this,                                  // Object to call on
		&AProjectileRocket::DestroyTimerFinished, // Member function pointer
		DestroyTime,                           // Time (seconds)
		false                                  // Loop (false = only once)
	);


	if (RocketMesh) RocketMesh->SetVisibility(false);
	if (CollisionBox) CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (TracerComponent) TracerComponent->Deactivate();
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())  ProjectileLoopComponent->Stop();
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}
