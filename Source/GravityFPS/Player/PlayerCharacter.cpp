#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "GravityFPS/Weapon/Weapon.h"
#include "GravityFPS/GravityFPSComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "PlayerAnimInstance.h"
#include "Engine/EngineTypes.h" 
#include "GravityFPS/GravityFPS.h"
#include "GravityFPS/PlayerController/CharacterPlayerController.h"
#include "GravityFPS/Gamemode/GravityFPSGamemode.h"
#include "TimerManager.h"
#include "GravityFPS/PlayerState/CharacterPlayerState.h"
#include "GravityFPS/Weapon/WeaponTypes.h"
#include "GravityFPS/Weapon/Magazine.h"
#include "GravityFPS/GravityFPSComponents/BuffComponent.h"
#include "Components/BoxComponent.h"
#include "GravityFPS/GravityFPSComponents/LagCompensationComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GravityFPS/GameState/GravityFPSGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GravityFPS/Weapon/Projectile.h"


// Sets default values
APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;


	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->SocketOffset = FVector(0.0f, 75.0f, 25.0f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bDoCollisionTest = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;


	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;

	//CurrHealth = MaxHealth;


	//OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	//OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));
	

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
	//UE_LOG(LogTemp, Warning, TEXT("Constructor - Camera: %p, Class: %s"), FollowCamera, *GetClass()->GetName());

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	/*
	* Hit boxes for server-side rewind
	*/

	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	for (auto& Box : HitCollisionBoxes) {
		if (Box.Value) {
			Box.Value->SetCollisionObjectType(ECC_Hitbox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	BuffSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BuffSpawnPoint"));
	BuffSpawnPoint->SetupAttachment(GetMesh(), FName("spine_03"));
	BuffSpawnPoint->SetRelativeLocation(FVector::ZeroVector);

	// --- Movement Buff Effect ---
	MovementBuffEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MovementBuffEffect"));
	MovementBuffEffectComponent->SetupAttachment(BuffSpawnPoint); // or to a socket/bone if you prefer
	MovementBuffEffectComponent->SetAutoActivate(false); // Only activate on buff
	// Don't set the asset here, do it in BeginPlay

	// --- Power Buff Effect ---
	PowerBuffEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PowerBuffEffect"));
	PowerBuffEffectComponent->SetupAttachment(BuffSpawnPoint); // or to a different socket/bone if desired
	PowerBuffEffectComponent->SetAutoActivate(false);

	ThrusterPSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ThrusterPSC"));
	ThrusterPSC->SetupAttachment(GetMesh(), TEXT("spine_03")); 
	ThrusterPSC->bAutoActivate = false; 
	ThrusterPSC->SetRelativeLocation(FVector(0, 0, 0));
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APlayerCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(APlayerCharacter, AO_Yaw);
	DOREPLIFETIME(APlayerCharacter, TurningInPlace);
	DOREPLIFETIME(APlayerCharacter, ReplicatedPitch);
	DOREPLIFETIME(APlayerCharacter, Health);
	DOREPLIFETIME(APlayerCharacter, Shield);
	DOREPLIFETIME(APlayerCharacter, bDisableGameplay);
	DOREPLIFETIME(APlayerCharacter, FlyingState);
	DOREPLIFETIME(APlayerCharacter, LastDamager)
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnDefaultWeapon();
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();

	if (APlayerController* PC = Cast<APlayerController>(Controller)) {
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer()) {
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
				Subsystem->AddMappingContext(PlayerMappingContext, 0);
				bInputsSet = true;
			}
		}
	}

	if (HasAuthority()) {
		OnTakeAnyDamage.AddDynamic(this, &APlayerCharacter::RecieveDamage);
		if (GetCapsuleComponent()) {
			GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::OnBeginOverlap);
		}
	}

	if (MovementBuffEffectComponent && MovementBuffEffectSystem)
	{
		MovementBuffEffectComponent->SetAsset(MovementBuffEffectSystem);
	}
	if (PowerBuffEffectComponent && PowerBuffEffectSystem)
	{
		PowerBuffEffectComponent->SetAsset(PowerBuffEffectSystem);
	}

	if (ThursterVFX && ThrusterPSC)
	{
		ThrusterPSC->SetTemplate(ThursterVFX);
	}
}

void APlayerCharacter::UpdateHUDHealth()
{
	CharacterPlayerController = !CharacterPlayerController ? Cast<ACharacterPlayerController>(Controller) : CharacterPlayerController;
	if (CharacterPlayerController) {
		CharacterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void APlayerCharacter::UpdateHUDShield()
{
	CharacterPlayerController = !CharacterPlayerController ? Cast<ACharacterPlayerController>(Controller) : CharacterPlayerController;
	if (CharacterPlayerController) {
		CharacterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void APlayerCharacter::UpdateHUDAmmo()
{
	CharacterPlayerController = !CharacterPlayerController ? Cast<ACharacterPlayerController>(Controller) : CharacterPlayerController;
	if (CharacterPlayerController && Combat && Combat->EquippedWeapon) {
		CharacterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		CharacterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmoLeft());
	}
}

void APlayerCharacter::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();
	if (World && HasAuthority() && DefaultWeaponClass && !bKilled) {
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		if (Combat) {
			Combat->EquipWeapon(StartingWeapon);
			if (Combat->EquippedWeapon && IsLocallyControlled()) {
				Combat->BaseRecoilPitch = Combat->EquippedWeapon->GetRecoilPitch();
				Combat->BaseRecoilYaw = Combat->EquippedWeapon->GetRecoilYaw();
			}
		}
	}
}

void APlayerCharacter::ActivateMovementBuffEffect()
{
	if (MovementBuffEffectComponent)
	{
		MovementBuffEffectComponent->Activate(true);
	}
}

void APlayerCharacter::DeactivateMovementBuffEffect()
{
	if (MovementBuffEffectComponent)
	{
		MovementBuffEffectComponent->Deactivate();
	}
}

void APlayerCharacter::ActivatePowerBuffEffect()
{
	if (PowerBuffEffectComponent)
	{
		PowerBuffEffectComponent->Activate(true);
	}
}

void APlayerCharacter::DeactivatePowerBuffEffect()
{
	if (PowerBuffEffectComponent)
	{
		PowerBuffEffectComponent->Deactivate();
	}
}


void APlayerCharacter::MulticastGainedTheLead_Implementation()
{
	if (!CrownSystem) return;
	if (!CrownSystemComponent) {
		CrownSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetCapsuleComponent(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if (CrownSystemComponent) {
		CrownSystemComponent->Activate();
	}
}

void APlayerCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownSystemComponent) {
		CrownSystemComponent->DestroyComponent();
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !bInputsSet && Controller)
	{
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC)
		{
			if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer()) {
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
					Subsystem->AddMappingContext(PlayerMappingContext, 0);
					bInputsSet = true;
				}
			}
		}
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy) {
		AimOffset(DeltaTime);
	}
	else {
		SimProxiesTurn();
	}

	ResetRecoilOffset(DeltaTime);

	HideCameraIfCharacterClose();
	PollInit();

	if (IsLocallyControlled()) DrawGravityBar();
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		if (MoveAction) {
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		}
		if (LookAction) {
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		}
		/*
		if (JumpAction) {
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Jump);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJumping);
		}
		*/
		if (ShootAction) {
			EnhancedInput->BindAction(ShootAction, ETriggerEvent::Started, this, &APlayerCharacter::PullTrigger);
			EnhancedInput->BindAction(ShootAction, ETriggerEvent::Completed, this, &APlayerCharacter::ReleaseTrigger);
		}
		if (EquipAction) {
			EnhancedInput->BindAction(EquipAction, ETriggerEvent::Started, this, &APlayerCharacter::Equip);
		}
		if (SwapAction) {
			EnhancedInput->BindAction(SwapAction, ETriggerEvent::Triggered, this, &APlayerCharacter::SwapWeapons);
		}
		if (ZoomAction) {
			EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Started, this, &APlayerCharacter::AimPressed);
			EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Completed, this, &APlayerCharacter::AimReleased);
		}
		if (ReloadAction) {
			EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &APlayerCharacter::Reload);
		}
		if (FlyAction) {
			EnhancedInput->BindAction(FlyAction, ETriggerEvent::Started, this, &APlayerCharacter::ChangeFlyingState);
		}
		if (FallAction) {
			EnhancedInput->BindAction(FallAction, ETriggerEvent::Started, this, &APlayerCharacter::Fall);
		}
		// Add gravity input
	}
}

void APlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat){
		Combat->InitializeComponentReferences(this, FollowCamera);
	} 
	if (Buff) {
		Buff->Character = this;
		Buff->SetInitalSpeed(GetCharacterMovement()->MaxWalkSpeed);
	}
	if (LagCompensation) {
		LagCompensation->Character = this;
		if (Controller) 
			LagCompensation->Controller = Cast<ACharacterPlayerController>(Controller);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value) {
	if (bDisableGameplay) return;

	FVector2D MovementVector = Value.Get<FVector2D>();
	if (Controller && MovementVector != FVector2D::ZeroVector)
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, ControlRotation.Yaw, 0);

		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDir, MovementVector.X);
		AddMovementInput(RightDir, MovementVector.Y);
	}

}

void APlayerCharacter::Look(const FInputActionValue& Value) {

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller) {
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(-LookAxisVector.Y);
	}
}

void APlayerCharacter::Jump()
{
	if (bDisableGameplay) return;
	Super::Jump();
}

void APlayerCharacter::PullTrigger() {
	if (bDisableGameplay) return;
	if (Combat) Combat->FireButtonPressed(true);
}

void APlayerCharacter::ReleaseTrigger()
{
	if (bDisableGameplay) return;
	if (Combat) Combat->FireButtonPressed(false);
}

void APlayerCharacter::Equip()
{
	if (bDisableGameplay) return;

	if (Combat && Combat->CombatState == ECombatState::ECS_Unoccupied)
	{
		ServerEquipWeapon(OverlappingWeapon);
	}
}

void APlayerCharacter::SwapWeapons()
{
	if (bDisableGameplay || !Combat || !Combat->ShouldSwapWeapons()) return;


	if (Combat->CombatState == ECombatState::ECS_Unoccupied)
	{
		if (HasAuthority())
		{
			Combat->SwapWeapons();
		}
		else
		{
			Combat->SwapWeapons();
			ServerSwapWeapons();
		}
	}
}



void APlayerCharacter::ServerSwapWeapons_Implementation()
{
	if (Combat && Combat->ShouldSwapWeapons())
	{
		Combat->SwapWeapons();
	}
}

void APlayerCharacter::AimPressed()
{
	if (bDisableGameplay) return;
	if (Combat) {
		Combat->SetAiming(true);
	}
}

void APlayerCharacter::AimReleased()
{
	if (bDisableGameplay) return;
	if (Combat) {
		Combat->SetAiming(false);
	}
}

void APlayerCharacter::Reload()
{
	if (bDisableGameplay) return;
	if (Combat) Combat->Reload();
}

void APlayerCharacter::Fall()
{
	if (bDisableGameplay) return;
	if (IsLocallyControlled() && !HasAuthority()) {
		ResetFlying();
	}
	ServerResetFlying();
}

void APlayerCharacter::RecieveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	float DamageToHealth = Damage;

	if (Shield > 0.f) {
		float DamageAbsorbed = FMath::Min(Shield, Damage);
		Shield = FMath::Clamp(Shield - DamageAbsorbed, 0.f, MaxShield);
		DamageToHealth = Damage - DamageAbsorbed;
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health == 0.f) {
		AGravityFPSGamemode* GravityFPSGamemode = GetWorld()->GetAuthGameMode<AGravityFPSGamemode>();
		if (GravityFPSGamemode) {
			CharacterPlayerController = !CharacterPlayerController ? Cast<ACharacterPlayerController>(Controller) : CharacterPlayerController;
			ACharacterPlayerController* AttackerController = Cast<ACharacterPlayerController>(InstigatorController);
			GravityFPSGamemode->PlayerKilled(this, CharacterPlayerController, AttackerController);
		}
	}
	else if (InstigatorController && InstigatorController){
		LastDamager = Cast<ACharacterPlayerController>(InstigatorController);

		GetWorldTimerManager().ClearTimer(LastDamagerResetTimer); // Clear any existing timer
		GetWorldTimerManager().SetTimer(
			LastDamagerResetTimer,
			this,
			&APlayerCharacter::ClearLastDamager,
			LastDamagerResetTime
		);
	}
}

bool APlayerCharacter::isLocallyReloading() const
{
	return Combat && Combat->bLocallyReloading;
}

ECombatState APlayerCharacter::GetCombatState() const
{
	if (!Combat) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

AWeapon* APlayerCharacter::GetWeapon() const
{
	return Combat == nullptr ? nullptr : Combat->EquippedWeapon;
}

FString APlayerCharacter::GetWeaponName() const
{
	if (!Combat || !Combat->EquippedWeapon) return FString(TEXT("None"));
	return Combat->EquippedWeapon->GetWeaponName();
}

void APlayerCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	AWeapon* LastWeapon = OverlappingWeapon; // Cache the old one

	OverlappingWeapon = Weapon;

	if (IsValid(LastWeapon))
	{
		LastWeapon->ShowPickupWidget(false); // Hide old widget
	}

	if (IsLocallyControlled() && IsValid(OverlappingWeapon))
	{
		OverlappingWeapon->ShowPickupWidget(true); // Show new widget
	}
}

bool APlayerCharacter::isAiming()
{
	return (Combat && Combat->bAiming);
}


void APlayerCharacter::PlayFireMontage(bool bIsAiming)
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && FireWeaponMontage) {
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bIsAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void APlayerCharacter::PlayReloadMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ReloadMontage) {
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssultRifle:
			SectionName = FName(TEXT("Rifle"));
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName(TEXT("Rifle"));
			break;
		case EWeaponType::EWT_Small:
			SectionName = FName(TEXT("Pistol"));
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName(TEXT("Rifle"));
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName(TEXT("Rifle"));
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void APlayerCharacter::PlayDeathMontage(bool bIsAiming)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && DeathMontage) {
		AnimInstance->Montage_Play(DeathMontage);
		FName SectionName;
		SectionName = bIsAiming ? FName("Aiming") : FName("NotAiming");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void APlayerCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage) {
		AnimInstance->Montage_Play(SwapMontage);
	}
}


void APlayerCharacter::PlayHitReactMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactMontage) {
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void APlayerCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (Health < LastHealth) {
		PlayHitReactMontage();
	}
}

void APlayerCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (Shield < LastShield) {
		PlayHitReactMontage();
	}
}

FVector APlayerCharacter::GetHitTarget() const
{
	if (!Combat) return FVector();
	return Combat->HitTarget;
}

void APlayerCharacter::Die(bool bPlayerLeftGame)
{
	if (bKilled) return;


	//if (HasAuthority()) MulticastResetGravityTimer();

	if (Combat) {
		DropOrDestoryWeapon(Combat->EquippedWeapon);
		DropOrDestoryWeapon(Combat->SecondaryWeapon);
	}

	MulticastDie(bPlayerLeftGame);
}

void APlayerCharacter::DropOrDestoryWeapon(AWeapon* Weapon)
{
	if (!Weapon) return;
	
	if (Weapon->bDestroyWeapon) {
		Weapon->DestroyMag();
		Weapon->Destroy();
	}
	else {
		Weapon->Dropped();
	}
}

void APlayerCharacter::MulticastDie_Implementation(bool bPlayerLeftGame)
{
	UE_LOG(LogTemp, Warning, TEXT("MulticastDie_Implementation: bPlayerLeftGame=%d, IsLocallyControlled=%d"), bPlayerLeftGame, IsLocallyControlled());
	bLeftGame = bPlayerLeftGame;

	if (IsLocallyControlled()) {
		StopCameraShake();
	}
	if (CharacterPlayerController) {
		CharacterPlayerController->SetHUDWeaponAmmo(0);
	}

	bKilled = true;
	PlayDeathMontage(isAiming());
	
	// Start Dissolve Effects
	DynamicDissolveMaterialInstances.SetNum(DissolveMaterialInstances.Num());
	for (int32 i = 0; i < DissolveMaterialInstances.Num(); ++i)
	{
		DynamicDissolveMaterialInstances[i] = UMaterialInstanceDynamic::Create(DissolveMaterialInstances[i], this);
		GetMesh()->SetMaterial(i, DynamicDissolveMaterialInstances[i]);
		DynamicDissolveMaterialInstances[i]->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstances[i]->SetScalarParameterValue(TEXT("Glow"), 150.f);
	}
	StartDissolve();
	// Disabling Gameplay
	bDisableGameplay = true;
	if (Combat) Combat->FireButtonPressed(false);

	// Disable Character Movement
	GetCharacterMovement()->DisableMovement();
	bDisableGameplay = true;
	GetCharacterMovement()->StopMovementImmediately();

	// Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	bool StopShowingScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (StopShowingScope)
		ShowSniperScopeWidget(false);

	if (CrownSystemComponent) {
		CrownSystemComponent->DestroyComponent();
	}

	GetWorldTimerManager().SetTimer(
		DeadTimer,
		this,
		&APlayerCharacter::DeadTimerFinished,
		DeadDelay
	);
}

void APlayerCharacter::DeadTimerFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("DeadTimerFinished: bLeftGame=%d, IsLocallyControlled=%d"), bLeftGame, IsLocallyControlled());
	AGravityFPSGamemode* GravityFPSGamemode = GetWorld()->GetAuthGameMode<AGravityFPSGamemode>();

	if (GravityFPSGamemode && !bLeftGame) {
		GravityFPSGamemode->RequestRespawn(this, Controller);
	} if (bLeftGame && IsLocallyControlled()) {
		UE_LOG(LogTemp, Warning, TEXT("Broadcasting OnLeftGame in DeadTimerFinished"));
		OnLeftGame.Broadcast();
	}
}

void APlayerCharacter::ServerLeaveGame_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("ServerLeaveGame_Implementation called"));
	AGravityFPSGamemode* GravityFPSGamemode = GetWorld()->GetAuthGameMode<AGravityFPSGamemode>();
	CharacterPlayerState = !CharacterPlayerState ? GetPlayerState<ACharacterPlayerState>() : CharacterPlayerState;
	if (GravityFPSGamemode && CharacterPlayerState) {
		{
			GravityFPSGamemode->PlayerLeftGame(CharacterPlayerState);
		}
	}
}

void APlayerCharacter::UpdateDissolveMaterials(float DissolveValue)
{
	for (const auto DynamicMaterial : DynamicDissolveMaterialInstances)
	{
		DynamicMaterial->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void APlayerCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &APlayerCharacter::UpdateDissolveMaterials);
	if (DissolveCurve && DissolveTimeline) {
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void APlayerCharacter::AttachMagToHand()
{
	if (!Combat || !Combat->EquippedWeapon) return;
	auto MagActor = Combat->EquippedWeapon->GetMagActor();
	if (!MagActor) return;

	MagActor->SetSimulate(false);
	MagActor->SetCollision(ECollisionEnabled::NoCollision);
	MagActor->SetVisible(true); // Ensure it's visible

	// Attach the mag to the player's hand socket
	MagActor->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("mag_holder"));
}

void APlayerCharacter::DropMagFromHand()
{
	if (!Combat || !Combat->EquippedWeapon) return;
	auto MagActor = Combat->EquippedWeapon->GetMagActor();
	if (!MagActor) return;

	// Detach and enable physics/collision
	MagActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	MagActor->SetSimulate(true);
	MagActor->SetCollision(ECollisionEnabled::QueryAndPhysics);
}

void APlayerCharacter::AttachMagToGun()
{
	if (!Combat || !Combat->EquippedWeapon) return;
	auto MagActor = Combat->EquippedWeapon->GetMagActor();
	if (!MagActor) return;

	MagActor->SetSimulate(false);
	MagActor->SetCollision(ECollisionEnabled::NoCollision);
	MagActor->SetVisible(true);

	// Attach the mag back to the gun's "fakemag" socket
	MagActor->AttachToComponent(
		Combat->EquippedWeapon->GetWeaponMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("fakemag")
	);
}


void APlayerCharacter::ChangeFlyingState()
{
	if (bDisableGameplay || !bCanGravity) return;
	bool callLocally = !HasAuthority() && IsLocallyControlled();
	switch (FlyingState)
	{
	case EFlyingState::EFS_Idle:
		if (callLocally) InitiateHover();
		ServerInitiateHover();
		break;
	case EFlyingState::EFS_Hovering:
		if (callLocally) InitiateFlying();
		ServerInitiateFlying();
		break;
	case EFlyingState::EFS_Flying:
		if (callLocally) InitiateHover();
		ServerInitiateHover();
		break;
	}
}

void APlayerCharacter::OnRep_FlyingState()
{
	if (!IsLocallyControlled())	return;
	switch (FlyingState)
	{
	case EFlyingState::EFS_Idle: ResetFlying(); break;
	case EFlyingState::EFS_Hovering: InitiateHover(); break;
	case EFlyingState::EFS_Flying: InitiateFlying(); break;
	}
}

void APlayerCharacter::InitiateHover()
{
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (!CM) return;
	if (!CM->IsFalling()) { // Launch Player Up
		CM->Launch(HoverLaunchVelocity);
	}

	CM->Velocity *= .4f;
	CM->GravityScale = .12f; // needs tweaks.

	if (Combat) {
		Combat->SetGravityPowerMultiplier(2.f);
	}

	if (IsLocallyControlled())
	{
		StopCameraShake();
	}

	if (ThursterVFX && ThrusterPSC)
	{
		ThrusterPSC->Deactivate();
	}

	if (!GetWorldTimerManager().IsTimerActive(GravityTimer)) {
		StartGravityTimer();
	}
}

void APlayerCharacter::InitiateFlying()
{
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (!CM || !GetCamera()) return;

	// Direction Change
	FVector NewGravityDirection = GetCamera()->GetForwardVector();
	CM->SetGravityDirection(NewGravityDirection);
	CM->GravityScale = 1.f;
	CM->Velocity += CM->GetGravityDirection() * InitialFlyingKick;

	if (Combat) {
		Combat->SetGravityPowerMultiplier(4.f);
	}

	if (IsLocallyControlled()) 
	{
		StartCameraShake();
	}

	if (ThrusterPSC)
	{		
			FVector GravityDir = GetCharacterMovement()->GetGravityDirection();
			FVector Up = -GravityDir;
			FRotator Rot = Up.Rotation();
			ThrusterPSC->SetWorldRotation(Rot);
			ThrusterPSC->Activate();
	}
	
}

void APlayerCharacter::ResetFlying()
{
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (!CM || bDisableGameplay) return;

	CM->GravityScale = 1.f;
	CM->SetGravityDirection(FVector(0.f, 0.f, -1.f)); // base gravity direction

	if (Combat) {
		Combat->SetGravityPowerMultiplier(1.f);
	}

	if (IsLocallyControlled())
	{
		StopCameraShake();
	}

	if (ThrusterPSC)
	{
		ThrusterPSC->Deactivate();
	}
}

void APlayerCharacter::ServerInitiateHover_Implementation()
{
	FlyingState = EFlyingState::EFS_Hovering;
	MulticastInitiateHover();
	OnRep_FlyingState();
}

void APlayerCharacter::ServerInitiateFlying_Implementation()
{
	FlyingState = EFlyingState::EFS_Flying;
	MulticastInitiateFlying();
	OnRep_FlyingState();
}

void APlayerCharacter::ServerResetFlying_Implementation()
{
	FlyingState = EFlyingState::EFS_Idle;
	MulticastResetFlying();
	OnRep_FlyingState();
}

void APlayerCharacter::MulticastInitiateHover_Implementation()
{
	InitiateHover();
}

void APlayerCharacter::MulticastInitiateFlying_Implementation()
{
	InitiateFlying();
}

void APlayerCharacter::MulticastResetFlying_Implementation()
{
	ResetFlying();
}

void APlayerCharacter::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (Other && Other->IsA(AProjectile::StaticClass())) return;

	bCanGravity = true;
	if (GetWorldTimerManager().IsTimerActive(GravityTimer)) {
		GetWorldTimerManager().ClearTimer(GravityTimer);
		if (CharacterPlayerController) {
			CharacterPlayerController->SetHUDGravityBar(1.f);
		}
	}
    if (FlyingState != EFlyingState::EFS_Idle)
    {
		if (HasAuthority())
		{
			FlyingState = EFlyingState::EFS_Idle;
		}
		else if (IsLocallyControlled())
		{
			ResetFlying();
		}
		ServerResetFlying();
    }
}

void APlayerCharacter::GravityTimerFinished()
{
	Fall();
	bCanGravity = false;
	if (IsLocallyControlled()) StopCameraShake();
}

void APlayerCharacter::DrawGravityBar()
{
	CharacterPlayerController = !CharacterPlayerController ? Cast<ACharacterPlayerController>(Controller) : CharacterPlayerController;
	if (!CharacterPlayerController) return;
	float Percent = 0.f;
	if (GetWorldTimerManager().IsTimerActive(GravityTimer))
	{
		float TimeRemaining = GetWorldTimerManager().GetTimerRemaining(GravityTimer);
		Percent = TimeRemaining / GravityTime;
	}
	else
	{
		Percent = bCanGravity? 1.f : 0.f;
	}
	CharacterPlayerController->SetHUDGravityBar(Percent);
}

void APlayerCharacter::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->ActorHasTag("DeathVolume"))
	{
		// Use PlayerKilled to handle the death event!
		AGravityFPSGamemode* GameMode = GetWorld()->GetAuthGameMode<AGravityFPSGamemode>();
		if (GameMode)
		{
			// Pass nullptr as AttackerController (no attacker for environment death)
			GameMode->PlayerKilled(this,
				Cast<ACharacterPlayerController>(Controller), // Victim
				LastDamager // No attacker
			);
		}
	}
}

void APlayerCharacter::StartCameraShake()
{
	if (!FlyingCameraShakeClass) return;
	if (APlayerController* PC = Cast<APlayerController>(Controller)) {
		if (!FlyingShakeInstance)
		{
			FlyingShakeInstance = PC->PlayerCameraManager->StartCameraShake(FlyingCameraShakeClass, 1.f);
		}
	}
}

void APlayerCharacter::StopCameraShake()
{
	if (APlayerController* PC = Cast<APlayerController>(Controller)) {
		if (FlyingShakeInstance) {
			PC->PlayerCameraManager->StopCameraShake(FlyingShakeInstance, false);
			UE_LOG(LogTemp, Warning, TEXT("Stopped camera shake instance %p"), FlyingShakeInstance);
			FlyingShakeInstance = nullptr;
		}
		if (FlyingCameraShakeClass) {
			PC->PlayerCameraManager->StopAllInstancesOfCameraShake(FlyingCameraShakeClass, false);
			UE_LOG(LogTemp, Warning, TEXT("Stopped all camera shakes of class %s"), *FlyingCameraShakeClass->GetName());
		}
	}
}

void APlayerCharacter::SetLastDamager(ACharacterPlayerController* InController)
{
	LastDamager = InController;
}

void APlayerCharacter::ClearLastDamager()
{
	LastDamager = nullptr;
}

void APlayerCharacter::OnRep_OverlapingWeapon(AWeapon* LastWeapon)
{
	if (IsValid(OverlappingWeapon))
		OverlappingWeapon->ShowPickupWidget(true);

	if (IsValid(LastWeapon))
		LastWeapon->ShowPickupWidget(false);
}

void APlayerCharacter::ResetRecoilOffset(float DeltaTime)
{
	if (!IsLocallyControlled() || bDisableGameplay) return;

	if (RecoilOffset.X != 0.f) {
		float OldYaw = RecoilOffset.X;
		RecoilOffset.X = FMath::FInterpTo(RecoilOffset.X, 0.f, DeltaTime, RecoilRecoverySpeed);
		float DeltaYaw = RecoilOffset.X - OldYaw;
		AddControllerYawInput(DeltaYaw);
	}
	if (RecoilOffset.Y != 0.f) {
		float OldPitch = RecoilOffset.Y;
		RecoilOffset.Y = FMath::FInterpTo(RecoilOffset.Y, 0.f, DeltaTime, RecoilRecoverySpeed);
		float DeltaPitch = RecoilOffset.Y - OldPitch;
		AddControllerPitchInput(-DeltaPitch);
	}
}

void APlayerCharacter::AimOffset(float DeltaTime)
{
	if (!IsLocallyControlled()) return;
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();

	bool bIsInAir = GetCharacterMovement()->IsFalling();

	//if (IsLocallyControlled()) {
	if (Speed == 0.f && !bIsInAir) {
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning) {
			InterpAO_Yaw = AO_Yaw;
		}

		TurnInPlace(DeltaTime);
	}
	else {
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		InterpAO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	//}

	AO_Pitch = GetBaseAimRotation().Pitch;

	/*
	if (AO_Pitch > 90.f && !IsLocallyControlled()) {
		// Translating pitch for other clients, from range [270, 360) to [-90, 0]

		FVector2D InRange(270.f, 360.f), OutRange(-90.f, 0.f);

		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
	*/

	if (HasAuthority())
	{
		ReplicatedPitch = AO_Pitch;
	}
	else
	{
		ServerUpdatePitch(AO_Pitch);
	}

	if (AO_Pitch > 90.f)
	{
		FVector2D InRange(270.f, 360.f), OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void APlayerCharacter::SimProxiesTurn()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	bRotateRootBone = false;

}

void APlayerCharacter::PollInit()
{
	if (!CharacterPlayerState) {
		CharacterPlayerState = GetPlayerState<ACharacterPlayerState>();
		if (CharacterPlayerState) {
			CharacterPlayerState->AddToScore(0.f);
			CharacterPlayerState->AddToDeaths(0);
		}
	}

	AGravityFPSGameState* GravityFPSGameState = Cast<AGravityFPSGameState>(UGameplayStatics::GetGameState(this));
	if (GravityFPSGameState && GravityFPSGameState->TopScoringPlayers.Contains(CharacterPlayerState)) {
		MulticastGainedTheLead();
	}
}

void APlayerCharacter::ApplyRecoil(float Pitch, float Yaw)
{
	if (!IsLocallyControlled() || bDisableGameplay) return;
	AddControllerPitchInput(-Pitch);
	AddControllerYawInput(Yaw);
	RecoilOffset.X += Yaw;
	RecoilOffset.Y += Pitch;
}

void APlayerCharacter::StartGravityTimer()
{
	GetWorldTimerManager().SetTimer(
		GravityTimer,
		this,
		&APlayerCharacter::GravityTimerFinished,
		GravityTime
	);
}

void APlayerCharacter::ResetGravityTimer()
{
	bCanGravity = true;
	if (GetWorldTimerManager().IsTimerActive(GravityTimer)) {
		GetWorldTimerManager().ClearTimer(GravityTimer);
		StartGravityTimer();	
		if (CharacterPlayerController) {
			CharacterPlayerController->SetHUDGravityBar(1.f);
		}
	}

	if (IsLocallyControlled() && FlyingState == EFlyingState::EFS_Flying) {
		StopCameraShake();
		StartCameraShake();
	}
}

void APlayerCharacter::MulticastResetGravityTimer_Implementation()
{
	ResetGravityTimer();
}

void APlayerCharacter::TurnInPlace(float DeltaTime)
{
	if (!IsLocallyControlled()) return;
	if (bDisableGameplay) {
		ServerSetTurningInPlace(ETurningInPlace::ETIP_NotTurning);
		bUseControllerRotationYaw = false;
		return;
	}
	ETurningInPlace NewTurning = ETurningInPlace::ETIP_NotTurning;

	if (AO_Yaw > 90.f)
	{
		NewTurning = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		NewTurning = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != NewTurning)
	{
		TurningInPlace = NewTurning;

		if (!HasAuthority())
		{
			ServerSetTurningInPlace(NewTurning);
		}
	}

	// Interpolation logic for smoothing locally
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			if (!HasAuthority())
			{
				ServerSetTurningInPlace(TurningInPlace);
			}
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void APlayerCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;

	bool MeshesValid = 
		Combat && 
		Combat->EquippedWeapon && 
		Combat->EquippedWeapon->GetWeaponMesh() && 
		Combat->EquippedWeapon->GetMagActor() && 
		Combat->EquippedWeapon->GetMagActor()->MagMesh &&
		Combat->SecondaryWeapon &&
		Combat->SecondaryWeapon->GetWeaponMesh()
		&& Combat->SecondaryWeapon->GetMagActor() && 
		Combat->SecondaryWeapon->GetMagActor()->MagMesh;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraCloseThreshold) {
		GetMesh()->SetVisibility(false);
		if (MeshesValid) {
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
			Combat->EquippedWeapon->GetMagActor()->MagMesh->bOwnerNoSee = true;
		}
	}
	else {
		GetMesh()->SetVisibility(true);
		if (MeshesValid) {
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
			Combat->EquippedWeapon->GetMagActor()->MagMesh->bOwnerNoSee = false;
		}
	}
}

void APlayerCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}

void APlayerCharacter::ServerEquipWeapon_Implementation(AWeapon* Weapon)
{
	if (!Combat) return;

	if (OverlappingWeapon)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void APlayerCharacter::ServerSetTurningInPlace_Implementation(ETurningInPlace NewTurning)
{
	TurningInPlace = NewTurning;
}

void APlayerCharacter::ServerUpdatePitch_Implementation(float NewPitch)
{
	ReplicatedPitch = NewPitch;
}