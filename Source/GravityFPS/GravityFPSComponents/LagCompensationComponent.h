// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	FBoxInformation() : Location(FVector::ZeroVector), Rotation(FRotator::ZeroRotator), BoxExtent(FVector::ZeroVector) {}

	FBoxInformation(FVector InLocation, FRotator InRotation, FVector InBoxExtent) 
		: Location(InLocation), Rotation(InRotation), BoxExtent(InBoxExtent){}

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage {
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitboxInfo;

	UPROPERTY()
	APlayerCharacter* Character;
};

USTRUCT()
struct FServerSideRewindResult
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool bHitConfirmed;
	UPROPERTY()
	bool bHeadShot;
};

USTRUCT()
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<APlayerCharacter*, uint32> Headshots;
	UPROPERTY()
	TMap<APlayerCharacter*, uint32> Bodyshots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GRAVITYFPS_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class APlayerCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

	/*
	* Hitscan
	*/

	FServerSideRewindResult ServerSideRewind(
		class APlayerCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

	/*
	* Projectile
	*/
	FServerSideRewindResult ProjectileServerSideRewind(
		APlayerCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	/*
	* Shotgun
	*/

	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<APlayerCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		APlayerCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		APlayerCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<APlayerCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);
protected:
	virtual void BeginPlay() override;	
	void SaveFramePackage(FFramePackage& Package);
	void SaveFramePackage();
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);


	void CacheBoxPositions(APlayerCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(APlayerCharacter* HitCharacter, const FFramePackage& Package);
	void ResetBoxes(APlayerCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(APlayerCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	FFramePackage GetFrameToCheck(APlayerCharacter* HitCharacter, float HitTime);

	/*
	* Hitscan confirm hit
	*/

	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,
		APlayerCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation
	);

	/*
	* Projectile confirm hit
	*/

	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		APlayerCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	/*
	* Shotgun confirm hit
	*/

	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations
	);
	
private:
	UPROPERTY()
	APlayerCharacter* Character;

	UPROPERTY()
	class ACharacterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
