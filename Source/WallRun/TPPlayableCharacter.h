// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPPlayableCharacter.generated.h"

UCLASS()
class WALLRUN_API ATPPlayableCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components (C++)")
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components (C++)")
	class USpringArmComponent* SpringArm;

	// The minimum acceptable angle from the wall normal to the character's forward vector.
	// If the actual angle is smaller than this value, then wall run will not succeed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running (C++)", meta = (ClampMin="0.0", ClampMax="45.0"))
	float MinFacingAngle;

	// The maximum acceptable angle from the wall normal to the character's forward vector.
	// If the actual angle is larger than this value, then wall run will not succeed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running (C++)", meta = (ClampMin="45.0", ClampMax="90.0"))
	float MaxFacingAngle;
	
	// The angle between the running direction and the projected launch direction (projected on the horizontal plane.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running (C++)", meta = (ClampMin="0.0", ClampMax="90.0"))
	float HPlaneLaunchAngle;

	// Elevation angle between the horizontal plane and the launch direction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running (C++)", meta = (ClampMin="0.0", ClampMax="90.0"))
	float ZElevationLaunchAngle;

	// Maximum distance the character can run a straight line on the wall surface, before the gravity affects its Z-velocity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running (C++)", meta = (ClampMin="0.0"))
	float MaxDistance;

public:
	// Sets default values for this character's properties
	ATPPlayableCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnCollided(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void HandleOnLanded(const FHitResult& Hit);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Axis);
	void MoveRight(float Axis);

	void BeginJump();
	void EndJump();

	void BeginWallRun();
	void EndWallRun();

	// Runs at every tick; the "heart" that enforces wall running.
	void TickWallRunning();

private:
	// True if this character's running on a wall. False, otherwise.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Running (C++)", meta = (AllowPrivateAccess="true"))
	bool bIsRunningOnWall;

	// Indicates the character has made a jump. Required to activate wall run. Check out OnCollided.
	bool bJumped;

	// The direction, either left or right, along the wall when wall running.
	FVector CurrentRunDirection;

	// The timer to trigger gravity effect when the wall running distance reaches MaxDistance.
	FTimerHandle GravitySuspendTimer;

	// The direction this character *should* be facing. This isn't the same as the character's actual facing direction.
	FVector DesiredFacingDirection;

	// Current wall the character is running on.
	class ARunnableWall* Wall;
};
