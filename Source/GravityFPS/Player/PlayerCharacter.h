#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "GravityFPS/Types/TurningInPlace.h"
#include "GravityFPS/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "GravityFPS/Types/CombatState.h"
#include "PlayerCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class AWeapon;
class UCombatComponent;
class UBuffComponent;
class ULagCompensationComponent;

UCLASS()
class GRAVITYFPS_API APlayerCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	virtual void Jump() override;
	void PullTrigger();
	void ReleaseTrigger();
	void Equip();
	void SwapWeapons();
	void AimPressed();
	void AimReleased();
	void Reload();
	UFUNCTION()
	void RecieveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);


public:
	virtual void Tick(float DeltaTime) override;


	FORCEINLINE float GetYaw() const { return AO_Yaw; }
	FORCEINLINE float GetPitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE float GetReplicatedPitch() const { return ReplicatedPitch; }
	FORCEINLINE UCameraComponent* GetCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool isKilled() const { return bKilled; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	bool isLocallyReloading() const;
	ECombatState GetCombatState() const;
	AWeapon* GetWeapon() const;

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool isAiming();

	/*
	* Play Montages
	*/
	void PlayFireMontage(bool bIsAiming);
	void PlayReloadMontage();
	void PlayDeathMontage(bool bIsAiming);
	void PlaySwapMontage();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();
	FVector GetHitTarget() const;
	void Die();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie();

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	UPROPERTY()
	bool bFinishedSwapping = false;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	void SpawnDefaultWeapon();

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	USpringArmComponent* SpringArm;


	/*
	* Input
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ShootAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* EquipAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* SwapAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ZoomAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ReloadAction;



	UPROPERTY(ReplicatedUsing = OnRep_OverlapingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlapingWeapon(AWeapon* LastWeapon);


	/*
	* GravityFPS Components
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere)
	ULagCompensationComponent* LagCompensation;


	void AimOffset(float DeltaTime);

	void SimProxiesTurn();

	// Sends pitch from local client to server
	UFUNCTION(Server, Reliable)
	void ServerUpdatePitch(float NewPitch);

	// Poll for any relevant classes and initialze HUD
	void PollInit();

	void DropOrDestoryWeapon(AWeapon* Weapon);


	/*
	* Hit boxes for server-side rewind
	*/

	UPROPERTY(EditAnywhere)
	UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;


private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;
	
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(AWeapon* Weapon);
	UFUNCTION(Server, Reliable)
	void ServerSwapWeapons();
	UFUNCTION(Server, Reliable)
	void ServerSetTurningInPlace(ETurningInPlace NewTurning);


	UPROPERTY(Replicated)
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	UPROPERTY(Replicated)
	ETurningInPlace TurningInPlace;

	void TurnInPlace(float DeltaTime);

	/*
	* Montages
	*/
	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* DeathMontage;
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* SwapMontage;

	UPROPERTY(Replicated)
	float ReplicatedPitch;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraCloseThreshold = 200.f;

	void PlayHitReactMontage();

	bool bRotateRootBone;

	/*
	* Health
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/*
	* Shield
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	bool bKilled = false;

	UPROPERTY()
	class ACharacterPlayerController* CharacterPlayerController;

	FTimerHandle DeadTimer;

	UPROPERTY(EditDefaultsOnly)
	float DeadDelay = 3.f;

	void DeadTimerFinished();

	/*
	* Dissolve Effect
	*/

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;
	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterials(float DissolveValue);
	void StartDissolve();

	// Dynamic instance that change at runtime. 
	UPROPERTY(VisibleAnywhere, Category = "Killed")
	TArray<UMaterialInstanceDynamic*> DynamicDissolveMaterialInstances;
	// Material instance set on the blueprint used with the dynamic material instance. 
	UPROPERTY(EditAnywhere, Category = "Killed")
	TArray<UMaterialInstance*> DissolveMaterialInstances;

	UPROPERTY()
	class ACharacterPlayerState* CharacterPlayerState;

	bool bInputsSet = false;

	/*
	* Mag in and out functions
	*/
	UFUNCTION(BlueprintCallable)
	void AttachMagToHand();
	UFUNCTION(BlueprintCallable)
	void DropMagFromHand();
	UFUNCTION(BlueprintCallable)
	void AttachMagToGun();

	/*
	* Default Weapon
	*/

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;
};
