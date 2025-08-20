// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Magazine.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "WeaponSpawnPoint.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	SetRootComponent(WeaponMesh);
	if (bIsRL) {
		WeaponMesh->SetRelativeRotation(FRotator(0.f, 180.f, 0.f)); // Or try (0, 90, 0), (0, -90, 0) depending on which way it's flipped
	}

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EnableCustomDepth(true);
	SetCustomDepth(CUSTOM_DEPTH_PURPLE);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, WeaponMesh);
	DOREPLIFETIME(AWeapon, MagazineActor);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly)
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	static const TArray<FName> MagBoneNames = { TEXT("clip"), TEXT("magazine") };
	for (const FName& BoneName : MagBoneNames)
	{
		if (WeaponMesh->GetBoneIndex(BoneName) != INDEX_NONE)
		{
			WeaponMesh->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
		}
	}

	if (MagazineActor == nullptr && MagazineClass && HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		MagazineActor = GetWorld()->SpawnActor<AMagazine>(MagazineClass, SpawnParams);
		if (MagazineActor)
		{
			// Attach to weapon at fake mag socket
			MagazineActor->AttachToComponent(
				WeaponMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				TEXT("fakemag")
			);
		}
	}

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

	ShowPickupWidget(false);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (WeaponState == EWeaponState::EWS_Initial) Rotate(DeltaTime);
}

void AWeapon::Rotate(float DeltaTime)
{
	FVector DesiredLocation = GetActorLocation();
	FRotator DesiredRotation = GetActorRotation();

	const float Time = GetWorld()->GetTimeSeconds();

	FVector SwayOffset;

	SwayOffset.Z = FMath::Cos(Time * SwayFrequency) * SwayAmplitdue;

	DesiredLocation += SwayOffset;
	DesiredRotation.Yaw += SpinSpeed * DeltaTime;


	FVector NewLocation = FMath::VInterpTo(GetActorLocation(), DesiredLocation, DeltaTime, LocationLagSpeed);

	SetActorLocationAndRotation(NewLocation, DesiredRotation);
}

void AWeapon::SetHUDAmmo()
{
	OwnerPlayerCharacter = !OwnerPlayerCharacter ? Cast<APlayerCharacter>(GetOwner()) : OwnerPlayerCharacter;
	if (OwnerPlayerCharacter) {
		OwnerPlayerController = !OwnerPlayerController ? Cast<ACharacterPlayerController>(OwnerPlayerCharacter->Controller) : OwnerPlayerController;
		if (OwnerPlayerController) OwnerPlayerController->SetHUDWeaponAmmo(Ammo);
	}
}

void AWeapon::DestroyMag()
{
	if (MagazineActor) {
		MagazineActor->Destroy();
		MagazineActor = nullptr;
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, AmmoCapacity);
	SetHUDAmmo();
	if (HasAuthority()) ClientUpdateAmmo(Ammo);
	else if (OwnerPlayerCharacter && OwnerPlayerCharacter->IsLocallyControlled())++Sequence;
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, AmmoCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, AmmoCapacity);
	SetHUDAmmo();
}


void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
	
}


void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnableCustomDepth(false);

	OwnerPlayerCharacter = !OwnerPlayerCharacter ? Cast<APlayerCharacter>(GetOwner()) : OwnerPlayerCharacter;
	if (OwnerPlayerCharacter && bUseServerSideRewind) {
		OwnerPlayerController = !OwnerPlayerController ? Cast<ACharacterPlayerController>(OwnerPlayerCharacter->Controller) : OwnerPlayerController;
		if (OwnerPlayerController && HasAuthority() && !OwnerPlayerController->HighPingDelegate.IsBound()) OwnerPlayerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
	}

	if (WeaponSpawnPoint) {
		WeaponSpawnPoint->StartSpawnWeaponTimer();
		WeaponSpawnPoint = nullptr;
	}
}

void AWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponMesh) {
		//EnableCustomDepth(true);
		SetCustomDepth(CUSTOM_DEPTH_TAN);
	}

	OwnerPlayerCharacter = !OwnerPlayerCharacter ? Cast<APlayerCharacter>(GetOwner()) : OwnerPlayerCharacter;
	if (OwnerPlayerCharacter && bUseServerSideRewind) {
		OwnerPlayerController = !OwnerPlayerController ? Cast<ACharacterPlayerController>(OwnerPlayerCharacter->Controller) : OwnerPlayerController;
		if (OwnerPlayerController && HasAuthority() && OwnerPlayerController->HighPingDelegate.IsBound()) OwnerPlayerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}
}

void AWeapon::OnDropped()
{
	if (HasAuthority()) {
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	EnableCustomDepth(true);
	SetCustomDepth(CUSTOM_DEPTH_PURPLE);

	OwnerPlayerCharacter = !OwnerPlayerCharacter ? Cast<APlayerCharacter>(GetOwner()) : OwnerPlayerCharacter;
	if (OwnerPlayerCharacter && bUseServerSideRewind) {
		OwnerPlayerController = !OwnerPlayerController ? Cast<ACharacterPlayerController>(OwnerPlayerCharacter->Controller) : OwnerPlayerController;
		if (OwnerPlayerController && HasAuthority() && OwnerPlayerController->HighPingDelegate.IsBound()) OwnerPlayerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (IsValid(PickupWidget))
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnim)
		WeaponMesh->PlayAnimation(FireAnim, false);

	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	//FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	//WeaponMesh->DetachFromComponent(DetachRules);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
	OwnerPlayerCharacter = nullptr;
	OwnerPlayerController = nullptr;
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (!Owner) {
		OwnerPlayerCharacter = nullptr;
		OwnerPlayerController = nullptr;
	}
	else {
		OwnerPlayerCharacter = !OwnerPlayerCharacter ? Cast<APlayerCharacter>(Owner) : OwnerPlayerCharacter;
		if (OwnerPlayerCharacter && OwnerPlayerCharacter->GetWeapon() == this)
			SetHUDAmmo();
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	if (PlayerCharacter && PickupWidget) {
		PlayerCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	if (IsValid(PlayerCharacter)) {
		PlayerCharacter->SetOverlappingWeapon(nullptr);
	}
}


FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("Muzzle");
	if (!MuzzleSocket) return FVector();

	const FTransform SocketTransfrom = MuzzleSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransfrom.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

void AWeapon::EnableCustomDepth(bool Enable)
{
	if (WeaponMesh) WeaponMesh->SetRenderCustomDepth(Enable);
	if (MagazineActor && MagazineActor->MagMesh) MagazineActor->MagMesh->SetRenderCustomDepth(Enable);
}

void AWeapon::SetCustomDepth(int32 StencilValue)
{
	if (WeaponMesh) {
		WeaponMesh->SetCustomDepthStencilValue(StencilValue);
		WeaponMesh->MarkRenderStateDirty();
	}
	if (MagazineActor && MagazineActor->MagMesh) {
		MagazineActor->MagMesh->SetCustomDepthStencilValue(StencilValue);
		MagazineActor->MagMesh->MarkRenderStateDirty();
	}
}
