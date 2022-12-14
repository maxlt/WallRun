// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RunnableWall.generated.h"

UCLASS()
class WALLRUN_API ARunnableWall : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARunnableWall();

	// Returns true if the hit impact is on the facing side of the wall.
	UFUNCTION(BlueprintCallable, Category = "Wall Running (C++)")
	bool IsOnFacingSide(const FVector& HitImpact, const FVector& HitNormal) const;

	// Returns the world-space direction that is the facing side of the wall, typically local X-axis.
	UFUNCTION(BlueprintCallable, Category = "Wall Running (C++)")
	virtual FVector FacingSide() const;
};
