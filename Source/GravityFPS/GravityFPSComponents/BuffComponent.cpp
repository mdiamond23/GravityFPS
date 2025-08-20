
#include "BuffComponent.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CombatComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	
}

void UBuffComponent::SetInitalSpeed(float BaseSpeed)
{
	InitialBaseSpeed = BaseSpeed;
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::ReplenishShield(float ShieldAmount)
{
	if (!Character) return;
	Character->SetShield(FMath::Clamp(Character->GetShield() + ShieldAmount, 0.f, Character->GetMaxShield()));
	Character->UpdateHUDShield();
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || !Character || Character->isKilled()) return;

	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));
	AmountToHeal -= HealThisFrame;
	Character->UpdateHUDHealth();

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth()) {
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::BuffMovement(float BuffBaseSpeed, float BuffTime)
{
	if (!Character) return;

	Character->GetWorldTimerManager().SetTimer(
		MovementBuffTimer, 
		this, 
		&UBuffComponent::ResetMovement, 
		BuffTime);

	if (Character->GetCharacterMovement()) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
	}
	MulticastMovementBuff(BuffBaseSpeed, true);
}

void UBuffComponent::ResetMovement()
{
	if (!Character || !Character->GetCharacterMovement()) return;
	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	MulticastMovementBuff(InitialBaseSpeed, false);
}

void UBuffComponent::MulticastMovementBuff_Implementation(float BaseSpeed, bool bStart)
{
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	if (bStart) {
		Character->ActivateMovementBuffEffect();
	}
	else {
		Character->DeactivateMovementBuffEffect();
	}
}

void UBuffComponent::BuffPower(float BuffBasePower, float BuffTime)
{
	if (!Character || !Character->GetCombat()) return;

	Character->GetWorldTimerManager().SetTimer(
		PowerBuffTimer,
		this,
		&UBuffComponent::ResetPower,
		BuffTime
	);

	Character->GetCombat()->SetPowerUpPowerMultiplier(BuffBasePower);
	MulticastPowerBuff(BuffBasePower, true);
}

void UBuffComponent::ResetPower()
{
	if (!Character || !Character->GetCombat()) return;
	Character->GetCombat()->SetPowerUpPowerMultiplier(1.f);
	MulticastPowerBuff(1.f, false);
}

void UBuffComponent::MulticastPowerBuff_Implementation(float PowerBuff, bool bStart)
{
	// Apply power buff
	Character->GetCombat()->SetPowerUpPowerMultiplier(PowerBuff);
	if (bStart) {
		Character->ActivatePowerBuffEffect();
	}
	else {
		Character->DeactivatePowerBuffEffect();
	}
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	HealRampUp(DeltaTime);
}