// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "GravityFPS/Weapon/Weapon.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"
#include "GravityFPS/Weapon/Shotgun.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	bCanFire = true;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character->HasAuthority()) {
		InitialzeCarriedAmmo();
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled()) {
		SetHUDCrosshairs(DeltaTime);
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (!Character || !EquippedWeapon) return;
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : NormalWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
		Character->ShowSniperScopeWidget(bIsAiming);
	if (Character->IsLocallyControlled()) 
		bAimButtonPressed = bIsAiming;
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed) {
		Fire();
	}
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::Fire()
{
	if (!CanFire()) return;

	bCanFire = false;

	if (Character && Character->IsLocallyControlled() && EquippedWeapon)
	{
		switch (EquippedWeapon->FireType)
		{
		case EFireType::EFT_Projectile:
			FireProjectileWeapon();
			break;
		case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
		case EFireType::EFT_Shotgun:
			FireShotgun();
			break;
		default:
			break;
		}
	}

	if (EquippedWeapon)
	{
		CrosshairShootFactor = .8f;
	}

	StartFireTimer();
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character) {
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (Character->IsLocallyControlled()) LocalFire(HitTarget);
		if (!Character->HasAuthority()) ServerFire(HitTarget, EquippedWeapon->FireDelay);
		else MulticastFire(HitTarget);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character) {
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (Character->IsLocallyControlled()) LocalFire(HitTarget);
		if (!Character->HasAuthority()) ServerFire(HitTarget, EquippedWeapon->FireDelay);
		else MulticastFire(HitTarget);
	}
}

void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		if (!Character->HasAuthority()) ShotgunLocalFire(HitTargets);
		ServerShotgunFire(HitTargets, Shotgun->FireDelay);
	}
}

bool UCombatComponent::CanFire() const
{
	return bCanFire && 
		EquippedWeapon && 
		!EquippedWeapon->IsEmpty() && 
		CombatState == ECombatState::ECS_Unoccupied && !bLocallyReloading;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
	if (Controller) Controller->SetHUDCarriedAmmo(CarriedAmmo);
}

void UCombatComponent::InitialzeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRPGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Small, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
}

void UCombatComponent::UpdateAmmoValues()
{
	if (!EquippedWeapon || !CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) return;

	int32 ReloadAmount = AmountToReload();
	CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
	CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

	Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
	if (Controller) Controller->SetHUDCarriedAmmo(CarriedAmmo);
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget) {
	if (!EquippedWeapon || !Character) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (!Shotgun || !Character) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay) {
	if (EquippedWeapon) {
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, .001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->IsLocallyControlled()) return;
	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}

bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay) {
	if (EquippedWeapon) {
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, .001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	ShotgunLocalFire(TraceHitTargets);
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;

	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld) {
		FVector Start = CrosshairWorldPosition;
		if (Character) {
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		FHitResult Hit;

		bool bHit = GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility
		);

		if (!bHit) {
			// if nothing was hit, fake a hit at the trace end point ***
			TraceHitResult.ImpactPoint = End;
		}

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
			bOverPlayer = true;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
			bOverPlayer = false;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (!Character || !Character->Controller) return;

	Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;

	if (Controller) {
		HUD = !HUD ? Cast<APlayerHUD>(Controller->GetHUD()) : HUD;
		if (HUD) {
			if (EquippedWeapon) {
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else {
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			// Calculate crosshair spread
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling()) {
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else {
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming) {
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, .3f, DeltaTime, 30.f);
			}
			else {
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			if (bOverPlayer) {
				CrosshairPlayerFactor = FMath::FInterpTo(CrosshairPlayerFactor, .3f, DeltaTime, 30.f);
			}
			else {
				CrosshairPlayerFactor = FMath::FInterpTo(CrosshairPlayerFactor, 0, DeltaTime, 30.f);
			}

			CrosshairShootFactor = FMath::FInterpTo(CrosshairShootFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrosshairSpread =
				.5f +
				CrosshairVelocityFactor + 
				CrosshairInAirFactor -
				CrosshairAimFactor + 
				CrosshairShootFactor - 
				CrosshairPlayerFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!EquippedWeapon || !Camera) return;

	if (bAiming) {
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else {
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}

	Camera->SetFieldOfView(CurrentFOV);
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon || !Character) return;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer, 
		this, 
		&UCombatComponent::FireTimerFinished, 
		EquippedWeapon->FireDelay
		);
}

void UCombatComponent::FireTimerFinished()
{
	if (!EquippedWeapon) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic) {
		Fire();
	}
	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}
}


void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	if (!Character || !EquippedWeapon) return;
	bAiming = bIsAiming;
	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : NormalWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
		Character->ShowSniperScopeWidget(bIsAiming);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (EquippedWeapon && !SecondaryWeapon) {
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else {
		EquipPrimaryWeapon(WeaponToEquip);
	}
	
}

void UCombatComponent::SwapWeapons()
{
	if (CombatState != ECombatState::ECS_Unoccupied || !Character) return;


	Character->PlaySwapMontage();
	CombatState = ECombatState::ECS_SwappingWeapons;
	Character->bFinishedSwapping = false;

	if (SecondaryWeapon)
		SecondaryWeapon->EnableCustomDepth(false);

	// Immediately swap locally for visual feedback (Client-side)
	FinishSwapAttachWeapons();  // Ensure local client visual state is correct immediately
}


void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip || CombatState == ECombatState::ECS_Reloading) return;
	if (EquippedWeapon) {
		EquippedWeapon->Dropped();
	}
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	FName SocketName = EquippedWeapon->IsRL() ? FName("GunHolder_RL") : FName("GunHolder");
	const USkeletalMeshSocket* GunHolder = Character->GetMesh()->GetSocketByName(SocketName);

	if (GunHolder)
	{
		GunHolder->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

	if (Character && Character->IsLocallyControlled())
	{
		EquippedWeapon->SetHUDAmmo();
		UpdateCarriedAmmo();
		Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
		if (Controller) Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	PlayWeaponEquipSound(EquippedWeapon);
	EquippedWeapon->ShowPickupWidget(false);
	EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	SecondaryWeapon->SetOwner(Character);
	AttachActorToBackPack(SecondaryWeapon);
	PlayWeaponEquipSound(SecondaryWeapon);
	SecondaryWeapon->ShowPickupWidget(false);
	SecondaryWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
}

void UCombatComponent::PlayWeaponEquipSound(AWeapon* Weapon)
{
	if (Weapon->EquipSound) {
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Weapon->EquipSound,
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::Reload()
{
	if (EquippedWeapon && EquippedWeapon->GetAmmoLeft() == EquippedWeapon->GetAmmoCapacity() && !bLocallyReloading) return;
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading) {
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!Character || !EquippedWeapon) return;

	UpdateAmmoValues();

	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled()) HandleReload();
}

void UCombatComponent::HandleReload()
{
	if (Character)
		Character->PlayReloadMontage();
}

void UCombatComponent::AttachActorToBackPack(AActor* ActorToAttach)
{
	if (!Character || !Character->GetMesh() || !ActorToAttach) return;
		
	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (BackpackSocket) BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (!Character || !Character->GetMesh() || !ActorToAttach) return;

	FName SocketName = Cast<AWeapon>(ActorToAttach)->IsRL() ? FName("GunHolder_RL") : FName("GunHolder");
	const USkeletalMeshSocket* GunHolderSocket = Character->GetMesh()->GetSocketByName(SocketName);

	if (GunHolderSocket)
	{
		GunHolderSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (!EquippedWeapon) return 0;
	if (!CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) return 0;
	int32 RoomInMag = EquippedWeapon->GetAmmoCapacity() - EquippedWeapon->GetAmmoLeft();
	int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	return FMath::Min(RoomInMag, AmountCarried);
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		break;
	case ECombatState::ECS_SwappingWeapons:
		if (Character && !Character->IsLocallyControlled())
			Character->PlaySwapMontage();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed) {
			Fire();
		}
		break;
	default:
		break;
	}
}


void UCombatComponent::FinishReloading()
{
	if (!Character) return;
	bLocallyReloading = false;
	if (Character->HasAuthority())
		CombatState = ECombatState::ECS_Unoccupied;
	if (bFireButtonPressed)
		Fire();
}

void UCombatComponent::FinishSwap()
{
	if (Character && Character->HasAuthority())
		CombatState = ECombatState::ECS_Unoccupied;

	if (Character)
		Character->bFinishedSwapping = true;

	if (SecondaryWeapon)
		SecondaryWeapon->EnableCustomDepth(true);
}

void UCombatComponent::FinishSwapAttachWeapons()
{
	// Only server performs authoritative multicast
	if (Character->HasAuthority())
	{
		Swap(EquippedWeapon, SecondaryWeapon);
		MulticastSwapWeapons();
	}
}

void UCombatComponent::MulticastSwapWeapons_Implementation()
{
	if (!Character) return;

	// Weapon pointers already swapped on server. Just attach properly here.
	if (EquippedWeapon)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		EquippedWeapon->SetOwner(Character);
	}

	if (SecondaryWeapon)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackPack(SecondaryWeapon);
		SecondaryWeapon->SetOwner(Character);
	}

	if (Character->IsLocallyControlled())
	{
		EquippedWeapon->SetHUDAmmo();
		UpdateCarriedAmmo();
		PlayWeaponEquipSound(EquippedWeapon);
	}
}

void UCombatComponent::InitializeComponentReferences(APlayerCharacter* InCharacter, UCameraComponent* InCamera)
{
	Character = InCharacter;
	Camera = InCamera;

	if (Camera)
	{
		DefaultFOV = Camera->FieldOfView;
		CurrentFOV = DefaultFOV;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (!Character || !EquippedWeapon) return;

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	FName SocketName = EquippedWeapon->IsRL() ? FName("GunHolder_RL") : FName("GunHolder");
	const USkeletalMeshSocket* GunHolder = Character->GetMesh()->GetSocketByName(SocketName);
	if (GunHolder)
	{
		GunHolder->AttachActor(EquippedWeapon, Character->GetMesh());
	}


	EquippedWeapon->SetOwner(Character);
	PlayWeaponEquipSound(EquippedWeapon);
	EquippedWeapon->ShowPickupWidget(false);
	EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquippedWeapon->SetHUDAmmo();
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (!Character || !SecondaryWeapon) return;

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackPack(SecondaryWeapon);
	SecondaryWeapon->SetOwner(Character);
	PlayWeaponEquipSound(SecondaryWeapon);
}

void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
		bAiming = bAimButtonPressed;
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (!EquippedWeapon) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = !Controller ? Cast<ACharacterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return (EquippedWeapon && SecondaryWeapon);
}
