// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayableCharacter.generated.h"

enum class EWallSide : uint8
{
	Left,
	Right
};


UCLASS()
class WALLRUN_API APlayableCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components (C++)")
	class UCameraComponent* CameraEye;

public:
	// Sets default values for this character's properties
	APlayableCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnWallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Scale);
	void MoveRight(float Scale);
	void Turn(float Scale);
	void LookUp(float Scale);
	void BeginJump();
	void EndJump();

	bool AreInputsValid() const;
	bool IsWallRunnable(const FHitResult& SurfaceWall) const;
	void UpdateWallRunning();
	FVector GetRunDirection(const FVector& SurfaceWallNormal) const;
	EWallSide GetWallSide(const FVector& SurfaceWallNormal) const;
	void BeginWallRunning();
	void EndWallRunning();

private:
	bool bIsRunningOnWall;
	float MoveForwardScale;
	float MoveRightScale;
	FVector DesiredWallRunDirection;
	EWallSide CurrentWallOn;
	bool bSetUpdateWallRunningToTick;
};
