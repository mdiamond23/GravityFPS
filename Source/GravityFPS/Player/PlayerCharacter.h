#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "GravityFPS/Types/TurningInPlace.h"
#include "GravityFPS/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "GravityFPS/Types/CombatState.h"
#include "GravityFPS/Types/FlyingState.h"
#include "PlayerCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class AWeapon;
class UCombatComponent;
class UBuffComponent;
class ULagCompensationComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class ACharacterPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

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
	void Fall();
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
	FORCEINLINE USceneComponent* GetBuffSpawnPoint() const { return BuffSpawnPoint; }
	FORCEINLINE EFlyingState GetFlyingState() const { return FlyingState; }
	bool isLocallyReloading() const;
	ECombatState GetCombatState() const;
	AWeapon* GetWeapon() const;
	FString GetWeaponName() const;

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
	void Die(bool bPlayerLeftGame);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie(bool bPlayerLeftGame);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	UPROPERTY()
	bool bFinishedSwapping = false;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	void SpawnDefaultWeapon();

	void ActivateMovementBuffEffect();
	void ActivatePowerBuffEffect();
	void DeactivateMovementBuffEffect();
	void DeactivatePowerBuffEffect();

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;


	FOnLeftGame OnLeftGame;

	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	void ApplyRecoil(float Pitch, float Yaw);

	void ResetGravityTimer();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetGravityTimer();

	void StartCameraShake();
	void StopCameraShake();

	void SetLastDamager(ACharacterPlayerController* InController);
	void ClearLastDamager();
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

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* FlyAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* FallAction;


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

	void ResetRecoilOffset(float DeltaTime);


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

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	//class UWidgetComponent* OverheadWidget;

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

	bool bLeftGame = false;

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

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;

	UPROPERTY(EditAnywhere)
	class UNiagaraComponent* CrownSystemComponent;

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

	/*
	* Weapon Recoil
	*/
	FVector2D RecoilOffset = FVector2D(0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Recoil")
	float RecoilRecoverySpeed = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class USceneComponent* BuffSpawnPoint;

	UPROPERTY(VisibleAnywhere, Category = "Effects")
	UNiagaraComponent* MovementBuffEffectComponent;

	UPROPERTY(VisibleAnywhere, Category = "Effects")
	UNiagaraComponent* PowerBuffEffectComponent;

	// Niagara system assets for each effect, assign these in Blueprint or details panel
	UPROPERTY(EditAnywhere, Category = "Effects")
	UNiagaraSystem* MovementBuffEffectSystem;

	UPROPERTY(EditAnywhere, Category = "Effects")
	UNiagaraSystem* PowerBuffEffectSystem;
	/*
	* Gravity Movement
	*/

	UPROPERTY(Replicated)
	EFlyingState FlyingState = EFlyingState::EFS_Idle;

	UPROPERTY(EditAnywhere, Category = "Gravity Movement")
	FVector HoverLaunchVelocity = FVector(0.f, 0.f, 100.f);

	void ChangeFlyingState();
	UFUNCTION()
	void OnRep_FlyingState();
	void InitiateHover();
	void StartGravityTimer();
	void InitiateFlying();
	void ResetFlying();

	UFUNCTION(Server, Reliable)
	void ServerInitiateHover();
	UFUNCTION(Server, Reliable)
	void ServerInitiateFlying();
	UFUNCTION(Server, Reliable)
	void ServerResetFlying();


	UFUNCTION(NetMulticast, Reliable)
	void MulticastInitiateHover();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastInitiateFlying();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetFlying();


	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
		bool bSelfMoved, FVector HitLocation, FVector HitNormal,
		FVector NormalImpulse, const FHitResult& Hit) override;

	FTimerHandle GravityTimer;

	void GravityTimerFinished();

	UPROPERTY(EditDefaultsOnly)
	float GravityTime = 3.f;

	UPROPERTY(EditDefaultsOnly)
	float InitialFlyingKick = 500.f;

	bool bCanGravity = true;

	void DrawGravityBar();


	/*
	* VFX
	*/

	UPROPERTY(EditAnywhere, Category = "VFX")
	class UParticleSystem* ThursterVFX;

	UPROPERTY()
	UParticleSystemComponent* ThrusterPSC;

	UPROPERTY(EditAnywhere, Category = "Camera Shake")
	TSubclassOf<UCameraShakeBase> FlyingCameraShakeClass;

	UPROPERTY()
	UCameraShakeBase* FlyingShakeInstance;

	UFUNCTION()
	void OnBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);


	/*
	* Last Damager Timer
	*/
	UPROPERTY(Replicated)
	ACharacterPlayerController* LastDamager;
	FTimerHandle LastDamagerResetTimer;
	float LastDamagerResetTime = 10.f;
};
