// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GravityFPS/Types/CombatState.h"
#include "GravityFPS/Weapon/Weapon.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!PlayerCharacter) {
		PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
	}
	if (!PlayerCharacter) return;

	FVector Velocity = PlayerCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bRotateRootBone = PlayerCharacter->ShouldRotateRootBone();

	bIsInAir = PlayerCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bAiming = PlayerCharacter->isAiming();

	bKilled = PlayerCharacter->isKilled();

	AO_Yaw = PlayerCharacter->GetYaw();
	AO_Pitch = PlayerCharacter->GetReplicatedPitch();

	EquippedWeapon = PlayerCharacter->GetWeapon();

	TurningInPlace = PlayerCharacter->GetTurningInPlace();

	FlyingState = PlayerCharacter->GetFlyingState();

	if (EquippedWeapon && EquippedWeapon->GetWeaponMesh() && PlayerCharacter->GetMesh()) {
		if (EquippedWeapon && EquippedWeapon->GetWeaponMesh() && PlayerCharacter->GetMesh()){
			LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);

			FVector OutPosition;
			FRotator OutRotation;
			PlayerCharacter->GetMesh()->TransformToBoneSpace(
				FName("hand_r"),
				LeftHandTransform.GetLocation(),
				FRotator::ZeroRotator,
				OutPosition,
				OutRotation
			);

			LeftHandTransform.SetLocation(OutPosition);
			LeftHandTransform.SetRotation(FQuat(OutRotation));

			if (PlayerCharacter->IsLocallyControlled()) {
				bLocallyControlled = true;
				FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - PlayerCharacter->GetHitTarget()));
				RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
			}
		}
	}

	bUseFABRIK = PlayerCharacter->GetCombatState() != ECombatState::ECS_Reloading && FlyingState != EFlyingState::EFS_Hovering;
	bool bFABRIKOverride = PlayerCharacter->IsLocallyControlled();
	if (bFABRIKOverride) bUseFABRIK = !PlayerCharacter->isLocallyReloading() && PlayerCharacter->bFinishedSwapping;
	bUseAimOffsets = PlayerCharacter->GetCombatState() != ECombatState::ECS_Reloading && !PlayerCharacter->bDisableGameplay;
	bTransformRightHand = PlayerCharacter->GetCombatState() != ECombatState::ECS_Reloading && !PlayerCharacter->bDisableGameplay;
}
