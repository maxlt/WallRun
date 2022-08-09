// Fill out your copyright notice in the Description page of Project Settings.

#include "RunnableWall.h"
#include "GameFramework/Character.h"

// Sets default values
ARunnableWall::ARunnableWall()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ARunnableWall::BeginPlay()
{
	Super::BeginPlay();
}

bool ARunnableWall::IsOnFacingSide(const FVector& Direction) const
{
	const auto temp = Direction | FacingSide();
	return temp >= -1.f && temp < 0.f;
}

FVector ARunnableWall::FacingSide() const
{
	return GetActorForwardVector();
}
