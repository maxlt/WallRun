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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Returns true if the Direction (world-space) points at the facing side, i.e. local X-axis, of the wall.
	// Facing it at 90deg or beyond will return false.
	bool IsOnFacingSide(const FVector& Direction) const;

	// Returns the world-space direction that is the facing side of the wall, i.e. local X-axis.
	virtual FVector FacingSide() const;
};
