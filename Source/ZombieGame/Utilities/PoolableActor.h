#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActor.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPoolableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * Optional hooks for actors managed by UActorPoolSubsystem. The pool already hides the actor and
 * turns off its collision on release; these are for anything else the actor keeps running
 * (movement components, timers, audio).
 */
class ZOMBIEGAME_API IPoolableActor
{
	GENERATED_BODY()

public:
	virtual void OnAcquiredFromPool() {}
	virtual void OnReleasedToPool() {}
};
