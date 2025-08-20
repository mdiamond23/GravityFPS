// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Inital State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX  UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),
	EFT_MAX  UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class GRAVITYFPS_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	void Rotate(float DeltaTime);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	void Dropped();
	void AddAmmo(int32 AmmoToAdd);
	virtual void OnRep_Owner() override;
	FVector TraceEndWithScatter(const FVector& HitTarget);

	/**
	*Texures for weapon crosshairs
	*/
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsBottom;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = .1f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class USoundWave* EquipSound;

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere, Category = "WeaponScatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType;

	/*
	* Enable or disable custom depth
	*/

	void EnableCustomDepth(bool Enable);
	void SetCustomDepth(int32 StencilValue);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped();

	UFUNCTION()
	void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	/*
	* Trace End With Scatter Parameters
	*/
	UPROPERTY(EditAnywhere, Category = "WeaponScatter")
	float DistanceToSphere = 800.f;
	UPROPERTY(EditAnywhere, Category = "WeaponScatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(EditAnywhere)
	float HeadshotMultiplier = 2.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind;

	UPROPERTY()
	class APlayerCharacter* OwnerPlayerCharacter;
	UPROPERTY()
	class ACharacterPlayerController* OwnerPlayerController;
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
private:

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnim;

	/*
	* Zoomed FOV while aiming
	*/
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 30.f;

	/*
	* Weapon Spinning
	*/

	UPROPERTY(EditAnywhere, Category = "Sway")
	float LocationLagSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Sway")
	float SwayAmplitdue = 2.f;

	UPROPERTY(EditAnywhere, Category = "Sway")
	float SwayFrequency = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Sway")
	float SpinSpeed = 60.f;

	/*
	* Ammo
	*/
	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 Ammo;
	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 AmmoCapacity;
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	// Number of unprocess server requests for ammo. Incremented in spend round, decremented in client update ammo
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(EditDefaultsOnly)
	bool bIsRL = false;

	UPROPERTY(Replicated)
	class AMagazine* MagazineActor;

	UPROPERTY(EditDefaultsOnly, Category = "Reload")
	TSubclassOf<AMagazine> MagazineClass;

	UPROPERTY(EditAnywhere)
	FString WeaponName = "";

	/*
	* Recoil values
	*/
	UPROPERTY(EditAnywhere, Category = "Recoil")
	float RecoilPitch = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Recoil")
	float RecoilYaw = .1f;


	UPROPERTY()
	class AWeaponSpawnPoint* WeaponSpawnPoint;
public:
	void SetWeaponState(EWeaponState State);
	void SetHUDAmmo();
	void DestroyMag();
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE int32 GetAmmoLeft() const { return Ammo; }
	FORCEINLINE int32 GetAmmoCapacity() const { return AmmoCapacity; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const{ return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE bool IsRL() const { return bIsRL; }
	FORCEINLINE AMagazine* GetMagActor() { return MagazineActor; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadshotMultiplier() const { return HeadshotMultiplier; }
	FORCEINLINE FString GetWeaponName() const { return WeaponName; }
	FORCEINLINE float GetRecoilPitch() const { return RecoilPitch; }
	FORCEINLINE float GetRecoilYaw() const { return RecoilYaw; }
	FORCEINLINE void SetWeaponSpawnPoint(AWeaponSpawnPoint* SpawnPoint) { WeaponSpawnPoint = SpawnPoint; }
};

