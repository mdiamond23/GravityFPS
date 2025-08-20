// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "GravityFPS/Player/PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GravityFPS/Weapon/Weapon.h"
#include "GravityFPS/GravityFPS.h"
#include "CombatComponent.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);
	
	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (const auto& YoungerPair : YoungerFrame.HitboxInfo) {
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitboxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerPair.Value;

		FBoxInformation InterpBoxInfo(
			FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction), // Interp Location
			FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction), // Interp Rotation
			YoungerBox.BoxExtent // Interp Box Extent
		);

		InterpFramePackage.HitboxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, APlayerCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (!HitCharacter) return FServerSideRewindResult();


	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);
	
	// Enable collision for head first
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECollisionResponse::ECR_Block);

	FHitResult ConfrimHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25;
	UWorld* World = GetWorld();


	if (World) {
		//ShowFramePackage(CurrentFrame, FColor::Blue);
		//DrawDebugLine(World, TraceStart, TraceEnd, FColor::Green, false, 4.0f, 0, 2.0f);

		World->LineTraceSingleByChannel(
			ConfrimHitResult,
			TraceStart,
			TraceEnd,
			ECC_Hitbox
		);

		if (ConfrimHitResult.bBlockingHit) {

			if (ConfrimHitResult.Component.IsValid()) {
				UBoxComponent* Box = Cast<UBoxComponent>(ConfrimHitResult.Component);
				if (Box) {
					//DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
				}
			}

			ResetBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, true };
		}
		else { // No headshot, check rest of the boxes
			for (const auto& Hitboxpair : HitCharacter->HitCollisionBoxes) {
				if (Hitboxpair.Value) {
					Hitboxpair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					Hitboxpair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECollisionResponse::ECR_Block);
					Hitboxpair.Value->UpdateOverlaps();
				}
			}

			World->LineTraceSingleByChannel(
				ConfrimHitResult,
				TraceStart,
				TraceEnd,
				ECC_Hitbox
			);

			if (ConfrimHitResult.bBlockingHit) {
				if (ConfrimHitResult.Component.IsValid()) {
					UBoxComponent* Box = Cast<UBoxComponent>(ConfrimHitResult.Component);
					if (Box) {
						//DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
					}
				}

				ResetBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	ResetBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, APlayerCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (!HitCharacter) return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable collision for head first
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECollisionResponse::ECR_Block);

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_Hitbox;
	PathParams.ActorsToIgnore.Add(GetOwner());
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);


	if (PathResult.HitResult.bBlockingHit) { // headshot, return early
		if (PathResult.HitResult.Component.IsValid()) {
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
			if (Box) {
				//DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
			}
		}

		ResetBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}
	else { // check rest of boxes
		for (const auto& Hitboxpair : HitCharacter->HitCollisionBoxes) {
			if (Hitboxpair.Value) {
				Hitboxpair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Hitboxpair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECollisionResponse::ECR_Block);
				Hitboxpair.Value->UpdateOverlaps();
			}
		}

		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
		if (PathResult.HitResult.bBlockingHit) { // Hit the body, return early
			if (PathResult.HitResult.Component.IsValid()) {
				UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
				if (Box) {
					//DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
				}
			}

			ResetBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, false };
		}
	}

	ResetBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}


FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	
	for (auto& Frame : FramePackages) {
		if (!Frame.Character) return FShotgunServerSideRewindResult();

		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);

		// Enable collision for head first
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECollisionResponse::ECR_Block);
	}

	UWorld* World = GetWorld();
	// Check for headshots
	for (auto& HitLocation: HitLocations) {
		FHitResult ConfrimHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25;

		World->LineTraceSingleByChannel(
			ConfrimHitResult,
			TraceStart,
			TraceEnd,
			ECC_Hitbox
		);
		
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(ConfrimHitResult.GetActor());
		if (PlayerCharacter) {
			if (ConfrimHitResult.Component.IsValid()) {
				UBoxComponent* Box = Cast<UBoxComponent>(ConfrimHitResult.Component);
				if (Box){
					//DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
				}
			}

			if (ShotgunResult.Headshots.Contains(PlayerCharacter)) ++ShotgunResult.Headshots[PlayerCharacter];
			else ShotgunResult.Headshots.Emplace(PlayerCharacter, 1);
		}
	}
	
	// Enable collision for all boxes then disable for the head
	for (auto & Frame : FramePackages){
		for (const auto& Hitboxpair : Frame.Character->HitCollisionBoxes) {
			if (Hitboxpair.Value) {
				Hitboxpair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Hitboxpair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECollisionResponse::ECR_Block);
				Hitboxpair.Value->UpdateOverlaps();
			}
		}
		
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Check for body shots
	for (auto& HitLocation : HitLocations) {
		FHitResult ConfrimHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25;

		World->LineTraceSingleByChannel(
			ConfrimHitResult,
			TraceStart,
			TraceEnd,
			ECC_Hitbox
		);

		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(ConfrimHitResult.GetActor());
		if (PlayerCharacter) {
			if (ConfrimHitResult.Component.IsValid()) {
				UBoxComponent* Box = Cast<UBoxComponent>(ConfrimHitResult.Component);
				if (Box) {
					//DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
				}
			}
			if (ShotgunResult.Bodyshots.Contains(PlayerCharacter)) ++ShotgunResult.Bodyshots[PlayerCharacter];
			else ShotgunResult.Bodyshots.Emplace(PlayerCharacter, 1);
		}
	}

	for (auto& Frame : CurrentFrames) {
		ResetBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotgunResult;
}



void ULagCompensationComponent::CacheBoxPositions(APlayerCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (!HitCharacter) return;

	for (const auto& HitboxPair : HitCharacter->HitCollisionBoxes) {
		if (HitboxPair.Value) {
			FBoxInformation BoxInfo(
				HitboxPair.Value->GetComponentLocation(),
				HitboxPair.Value->GetComponentRotation(),
				HitboxPair.Value->GetScaledBoxExtent()
			);
			OutFramePackage.HitboxInfo.Add(HitboxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(APlayerCharacter* HitCharacter, const FFramePackage& Package)
{
	if (!HitCharacter) return;

	for (const auto& HitboxPair : HitCharacter->HitCollisionBoxes) {
		if (HitboxPair.Value) {
			HitboxPair.Value->SetWorldLocation(Package.HitboxInfo[HitboxPair.Key].Location);
			HitboxPair.Value->SetWorldRotation(Package.HitboxInfo[HitboxPair.Key].Rotation);
			HitboxPair.Value->SetBoxExtent(Package.HitboxInfo[HitboxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetBoxes(APlayerCharacter* HitCharacter, const FFramePackage& Package)
{
	if (!HitCharacter) return;

	for (const auto& HitboxPair : HitCharacter->HitCollisionBoxes) {
		if (HitboxPair.Value) {
			HitboxPair.Value->SetWorldLocation(Package.HitboxInfo[HitboxPair.Key].Location);
			HitboxPair.Value->SetWorldRotation(Package.HitboxInfo[HitboxPair.Key].Rotation);
			HitboxPair.Value->SetBoxExtent(Package.HitboxInfo[HitboxPair.Key].BoxExtent);
			HitboxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(APlayerCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (!HitCharacter || !HitCharacter->GetMesh()) return;
	HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (const auto& BoxInfo : Package.HitboxInfo) {
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(
	APlayerCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation,
	float HitTime)
{
	return ConfirmHit(GetFrameToCheck(HitCharacter, HitTime), HitCharacter, TraceStart, HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(APlayerCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	return ProjectileConfirmHit(GetFrameToCheck(HitCharacter, HitTime), HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<APlayerCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (const auto& HitCharacter : HitCharacters) {
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(APlayerCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		!HitCharacter ||
		!HitCharacter->GetLagCompensation() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetHead() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetTail();
	if (bReturn) return FFramePackage();

	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;

	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	if (OldestHistoryTime > HitTime) return FFramePackage(); // Too old
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;

	while (Older->GetValue().Time > HitTime) // is older still younger than hit time?
	{
		// march back until Older < HitTime < Younger
		if (!Older->GetNextNode()) break;
		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime) // very unlikely, but found exact time
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}
	if (bShouldInterpolate)
	{
		// Interpolate between older and younger.
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}


void ULagCompensationComponent::ServerScoreRequest_Implementation(APlayerCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	

	if (Character && HitCharacter && Character->GetWeapon() && Confirm.bHitConfirmed) {
		float Damage = Character->GetCombat()->GetPowerMultiplier() * Character->GetWeapon()->GetDamage();
		Damage = Confirm.bHeadShot ? Damage * Character->GetWeapon()->GetHeadshotMultiplier() : Damage;
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage,
			Character->Controller,
			Character->GetWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(APlayerCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	if (Character && HitCharacter && Confirm.bHitConfirmed) {
		float Damage = Character->GetCombat()->GetPowerMultiplier() * Character->GetWeapon()->GetDamage();
		Damage = Confirm.bHeadShot ? Damage * Character->GetWeapon()->GetHeadshotMultiplier() : Damage;
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage,
			Character->Controller,
			Character->GetWeapon(),
			UDamageType::StaticClass()
		);
	}
}



void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<APlayerCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (auto& HitCharacter : HitCharacters) {
		if (!HitCharacter || !HitCharacter->GetWeapon() || !Character) continue;
		float TotalDamage = 0.f;
		if (Confirm.Headshots.Contains(HitCharacter))
			TotalDamage += Confirm.Headshots[HitCharacter] * HitCharacter->GetWeapon()->GetDamage() * HitCharacter->GetWeapon()->GetHeadshotMultiplier() * HitCharacter->GetCombat()->GetPowerMultiplier(); // Headshot Damage
		if (Confirm.Bodyshots.Contains(Character))
			TotalDamage += Confirm.Bodyshots[HitCharacter] * HitCharacter->GetWeapon()->GetDamage() * HitCharacter->GetCombat()->GetPowerMultiplier(); // Bodyshot Damage

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			HitCharacter->GetWeapon(),
			UDamageType::StaticClass()
		);
	}
}


void ULagCompensationComponent::SaveFramePackage()
{
	if (!Character || !Character->HasAuthority()) return;

	FFramePackage ThisFrame;
	SaveFramePackage(ThisFrame);

	if (FrameHistory.Num() <= 1)
	{
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FrameHistory.AddHead(ThisFrame);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = !Character ? Cast<APlayerCharacter>(GetOwner()) : Character;

	if (Character) {
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (const auto& BoxPair : Character->HitCollisionBoxes) {
			FBoxInformation BoxInformation(
				BoxPair.Value->GetComponentLocation(),
				BoxPair.Value->GetComponentRotation(),
				BoxPair.Value->GetScaledBoxExtent()
			);

			Package.HitboxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}