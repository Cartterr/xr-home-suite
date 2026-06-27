#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "XRHSValidationPawn.generated.h"

class UCameraComponent;
class UInstancedStaticMeshComponent;
class UMotionControllerComponent;
class UOculusXRPersistentPassthroughInstance;
class UTextRenderComponent;

enum class EXRHSDepthMode : uint8
{
	Off,
	Low,
	Adaptive
};

UCLASS()
class XRHSVALIDATION_API AXRHSValidationPawn : public APawn
{
	GENERATED_BODY()

public:
	AXRHSValidationPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMotionControllerComponent> LeftController;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMotionControllerComponent> RightController;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> GridLines;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> DebugPanel;

	UPROPERTY()
	TObjectPtr<UOculusXRPersistentPassthroughInstance> PassthroughInstance;

	void BuildValidationGrid();
	void CycleDepthMode();
	void ToggleDebugPanel();
	void ToggleHandsRequested();
	void TogglePassthroughPlacement();
	void ApplyPassthroughCompositingSettings();
	void TryStartPassthrough();
	void UpdateDebugPanel(float DeltaSeconds);

	UFUNCTION()
	void HandlePassthroughResumed();

	FString DepthModeText() const;
	FString TrackingText(const UMotionControllerComponent* MotionController) const;

	EXRHSDepthMode DepthMode = EXRHSDepthMode::Off;
	bool bPassthroughOverlayMode = true;
	bool bHandsRequested = true;
	bool bDebugPanelVisible = true;
	float SmoothedFrameMs = 0.0f;
	FString PassthroughState = TEXT("pending");
	FString CompositingState = TEXT("pending");
};
