// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityFPS/HUD/PlayerHUD.h"
#include "GravityFPS/Weapon/WeaponTypes.h"
#include "GravityFPS/Types/CombatState.h"
#include "CombatComponent.generated.h"

class AWeapon;
class APlayerCharacter;
class UCameraComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GRAVITYFPS_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class APlayerCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void EquipWeapon(AWeapon* WeaponToEquip);
	void SwapWeapons();
	void Reload();
	void InitializeComponentReferences(APlayerCharacter* InCharacter, UCameraComponent* InCamera);

	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(BlueprintCallable)
	void FinishSwap();
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();
	void FireButtonPressed(bool bPressed);
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	bool bLocallyReloading = false;
protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	

	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSwapWeapons();


	UFUNCTION(Server, Reliable, WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	void AttachActorToBackPack(AActor* ActorToAttach);
	void AttachActorToRightHand(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	int32 AmountToReload();

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);
	void PlayWeaponEquipSound(AWeapon* Weapon);
private:
	UPROPERTY()
	class APlayerCharacter* Character;
	UPROPERTY()
	class UCameraComponent* Camera;
	UPROPERTY()
	class ACharacterPlayerController* Controller;
	UPROPERTY()
	class APlayerHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();
	
	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;
	
	bool bAimButtonPressed = false;

	UFUNCTION()
	void OnRep_Aiming();

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AimWalkSpeed = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float NormalWalkSpeed = 800.f;

	bool bFireButtonPressed;

	/*
	* HUD and crosshairs
	*/

	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootFactor;
	float CrosshairPlayerFactor;

	FVector HitTarget;
	FHUDPackage HUDPackage;
	/*
	* Aiming and FOV
	*/
	float DefaultFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ZoomInterpSpeed = 30.f;

	void InterpFOV(float DeltaTime);

	bool bOverPlayer;

	/*
	* Automatic Fire 
	*/
	bool bCanFire = true;

	FTimerHandle FireTimer;


	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire() const;

	//Carried ammo for currently equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;
	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;
	UPROPERTY(EditAnywhere)
	int32 StartingRPGAmmo = 4;
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 24;
	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 12;
	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 6;
	void InitialzeCarriedAmmo();

	void UpdateAmmoValues();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	UFUNCTION()
	void OnRep_CombatState();

	UPROPERTY(VisibleAnywhere)
	float PowerMultiplier = 1.f;
	
public:
	bool ShouldSwapWeapons();

	FORCEINLINE bool CanSwap() const { return EquippedWeapon && SecondaryWeapon; }
	FORCEINLINE void SetPowerMultiplier(float Multiplier) { PowerMultiplier = Multiplier; }
	FORCEINLINE float GetPowerMultiplier() const { return PowerMultiplier; }
};
