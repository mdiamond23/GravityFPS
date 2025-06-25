// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Magazine.generated.h"

UCLASS()
class GRAVITYFPS_API AMagazine : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMagazine();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magazine")
	class UStaticMeshComponent* MagMesh;

	void SetSimulate(bool bSimulate);

	void SetCollision(ECollisionEnabled::Type CollisionType);

	void SetVisible(bool bVisible);

};
