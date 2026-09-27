#include "ZombieInputConfig.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace
{
	UInputAction* MakeAction(UObject& Outer, const TCHAR* Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = Outer.CreateDefaultSubobject<UInputAction>(Name);
		Action->ValueType = ValueType;
		return Action;
	}
}

UZombieInputConfig::UZombieInputConfig()
{
	// Every UObject built here is a default subobject: raw NewObject inside a constructor fatals the
	// editor on load (CLAUDE.md gotcha #2).
	Move = MakeAction(*this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	AimStick = MakeAction(*this, TEXT("IA_AimStick"), EInputActionValueType::Axis2D);
	Sprint = MakeAction(*this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	Fire = MakeAction(*this, TEXT("IA_Fire"), EInputActionValueType::Boolean);
	Reload = MakeAction(*this, TEXT("IA_Reload"), EInputActionValueType::Boolean);
	Interact = MakeAction(*this, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	WeaponSlot = MakeAction(*this, TEXT("IA_WeaponSlot"), EInputActionValueType::Axis1D);
	CycleWeapon = MakeAction(*this, TEXT("IA_CycleWeapon"), EInputActionValueType::Axis1D);
	Melee = MakeAction(*this, TEXT("IA_Melee"), EInputActionValueType::Boolean);
	Pause = MakeAction(*this, TEXT("IA_Pause"), EInputActionValueType::Boolean);

	// Pause must still fire while the game is paused, or the pause menu could never be closed by key.
	Pause->bTriggerWhenPaused = true;

	GameplayContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_Gameplay"));
	GlobalContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_Global"));

	BindMovement();
	BindCombat();
	BindWeaponSlots();
	BindGlobal();
}

void UZombieInputConfig::BindMovement()
{
	// D: raw axis lands on X, which is exactly "right" here - no modifier needed.
	GameplayContext->MapKey(Move, EKeys::D);
	{
		FEnhancedActionKeyMapping& Mapping = GameplayContext->MapKey(Move, EKeys::A);
		Mapping.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateA")));
	}
	{
		// W/S: swizzle the raw X output onto Y so they drive forward/back instead of left/right.
		FEnhancedActionKeyMapping& Mapping = GameplayContext->MapKey(Move, EKeys::W);
		UInputModifierSwizzleAxis* Swizzle = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleW"));
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	}
	{
		FEnhancedActionKeyMapping& Mapping = GameplayContext->MapKey(Move, EKeys::S);
		UInputModifierSwizzleAxis* Swizzle = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleS"));
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
		Mapping.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateS")));
	}
	{
		FEnhancedActionKeyMapping& Mapping = GameplayContext->MapKey(Move, EKeys::Gamepad_Left2D);
		Mapping.Modifiers.Add(CreateDefaultSubobject<UInputModifierDeadZone>(TEXT("MoveStickDeadZone")));
	}
	{
		FEnhancedActionKeyMapping& Mapping = GameplayContext->MapKey(AimStick, EKeys::Gamepad_Right2D);
		UInputModifierDeadZone* DeadZone = CreateDefaultSubobject<UInputModifierDeadZone>(TEXT("AimStickDeadZone"));
		DeadZone->LowerThreshold = 0.3f;
		Mapping.Modifiers.Add(DeadZone);
	}

	GameplayContext->MapKey(Sprint, EKeys::LeftShift);
	GameplayContext->MapKey(Sprint, EKeys::Gamepad_LeftTrigger);
	GameplayContext->MapKey(Sprint, EKeys::Gamepad_LeftThumbstick);
}

void UZombieInputConfig::BindCombat()
{
	GameplayContext->MapKey(Fire, EKeys::LeftMouseButton);
	GameplayContext->MapKey(Fire, EKeys::Gamepad_RightTrigger);

	GameplayContext->MapKey(Reload, EKeys::R);
	GameplayContext->MapKey(Reload, EKeys::Gamepad_FaceButton_Left);

	GameplayContext->MapKey(Interact, EKeys::E);
	GameplayContext->MapKey(Interact, EKeys::Gamepad_FaceButton_Bottom);

	GameplayContext->MapKey(Melee, EKeys::V);
	GameplayContext->MapKey(Melee, EKeys::Gamepad_FaceButton_Top);

	GameplayContext->MapKey(CycleWeapon, EKeys::MouseWheelAxis);
	GameplayContext->MapKey(CycleWeapon, EKeys::Gamepad_RightShoulder);
	{
		FEnhancedActionKeyMapping& Mapping = GameplayContext->MapKey(CycleWeapon, EKeys::Gamepad_LeftShoulder);
		Mapping.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("CycleNegateLB")));
	}
}

void UZombieInputConfig::BindWeaponSlots()
{
	// One action for every slot key: each key scales the action's value to its slot number, so
	// adding a slot key is one line and the handler never switches on keys.
	const FKey SlotKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(SlotKeys); ++Index)
	{
		FEnhancedActionKeyMapping& Mapping = GameplayContext->MapKey(WeaponSlot, SlotKeys[Index]);
		UInputModifierScalar* Scalar = CreateDefaultSubobject<UInputModifierScalar>(*FString::Printf(TEXT("SlotScalar%d"), Index + 1));
		Scalar->Scalar = FVector(static_cast<double>(Index + 1), 1.0, 1.0);
		Mapping.Modifiers.Add(Scalar);
	}
}

void UZombieInputConfig::BindGlobal()
{
	GlobalContext->MapKey(Pause, EKeys::Escape);
	GlobalContext->MapKey(Pause, EKeys::Gamepad_Special_Right);
}
