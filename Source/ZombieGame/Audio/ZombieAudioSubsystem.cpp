#include "ZombieAudioSubsystem.h"
#include "Audio/ZombieAudioSettings.h"
#include "Components/AudioComponent.h"
#include "Core/ZombieGameInstance.h"
#include "Core/ZombieGameState.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	constexpr float MixUpdateInterval = 0.2f;
}

UZombieAudioSubsystem* UZombieAudioSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UZombieAudioSubsystem>() : nullptr;
}

void UZombieAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	InWorld.GetTimerManager().SetTimer(MixTimer, this, &UZombieAudioSubsystem::UpdateMix, MixUpdateInterval, true);

	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(InWorld.GetGameInstance()))
	{
		SettingsChangedHandle = GameInstance->OnUserSettingsChanged.AddUObject(this, &UZombieAudioSubsystem::ApplyVolumeSettings);
	}
}

void UZombieAudioSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MixTimer);
		if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(World->GetGameInstance()))
		{
			GameInstance->OnUserSettingsChanged.Remove(SettingsChangedHandle);
		}
	}

	StopAllMusic();
	Super::Deinitialize();
}

float UZombieAudioSubsystem::GetCategoryVolume(EZombieSoundCategory Category) const
{
	const UZombieGameInstance* GameInstance = GetWorld() ? Cast<UZombieGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	if (!GameInstance)
	{
		return 1.0f;
	}

	const FZombieUserSettings& Settings = GameInstance->GetUserSettings();
	switch (Category)
	{
	case EZombieSoundCategory::Music:	return Settings.MasterVolume * Settings.MusicVolume;
	case EZombieSoundCategory::UI:		return Settings.MasterVolume * Settings.InterfaceVolume;
	default:							return Settings.MasterVolume * Settings.EffectsVolume;
	}
}

USoundBase* UZombieAudioSubsystem::PickVariant(const FZombieSoundSpec& Sound) const
{
	if (!Sound.IsSet())
	{
		return nullptr;
	}
	return Sound.Variants[FMath::RandRange(0, Sound.Variants.Num() - 1)].LoadSynchronous();
}

void UZombieAudioSubsystem::PlaySoundAtLocation(const FZombieSoundSpec& Sound, const FVector& Location)
{
	if (USoundBase* Variant = PickVariant(Sound))
	{
		const float Pitch = 1.0f + FMath::FRandRange(-Sound.PitchVariance, Sound.PitchVariance);
		UGameplayStatics::PlaySoundAtLocation(this, Variant, Location, Sound.Volume * GetCategoryVolume(Sound.Category), Pitch);
	}
}

void UZombieAudioSubsystem::PlaySound2D(const FZombieSoundSpec& Sound)
{
	if (USoundBase* Variant = PickVariant(Sound))
	{
		const float Pitch = 1.0f + FMath::FRandRange(-Sound.PitchVariance, Sound.PitchVariance);
		UGameplayStatics::PlaySound2D(this, Variant, Sound.Volume * GetCategoryVolume(Sound.Category), Pitch);
	}
}

void UZombieAudioSubsystem::PlayNamedSound2D(FName SoundName)
{
	if (const FZombieSoundSpec* Sound = UZombieAudioSettings::GetOrLoadDefault()->NamedSounds.Find(SoundName))
	{
		PlaySound2D(*Sound);
	}
}

void UZombieAudioSubsystem::PlayNamedSoundAtLocation(FName SoundName, const FVector& Location)
{
	if (const FZombieSoundSpec* Sound = UZombieAudioSettings::GetOrLoadDefault()->NamedSounds.Find(SoundName))
	{
		PlaySoundAtLocation(*Sound, Location);
	}
}

UAudioComponent* UZombieAudioSubsystem::StartLoop(const FZombieSoundSpec& Sound)
{
	USoundBase* Variant = PickVariant(Sound);
	if (!Variant)
	{
		return nullptr;
	}

	UAudioComponent* Component = UGameplayStatics::CreateSound2D(this, Variant, 1.0f, 1.0f, 0.0f, nullptr, false, false);
	if (Component)
	{
		Component->bIsUISound = true;
		Component->SetVolumeMultiplier(0.0f);
		Component->Play();
	}
	return Component;
}

void UZombieAudioSubsystem::StopAllMusic()
{
	for (UAudioComponent* Component : { PrimaryMusic.Get(), CombatMusic.Get(), Ambience.Get() })
	{
		if (IsValid(Component))
		{
			Component->Stop();
			Component->DestroyComponent();
		}
	}
	PrimaryMusic = nullptr;
	CombatMusic = nullptr;
	Ambience = nullptr;
}

void UZombieAudioSubsystem::SetMusicState(EZombieMusicState NewState)
{
	if (NewState == MusicState)
	{
		return;
	}

	MusicState = NewState;
	StopAllMusic();

	const UZombieAudioSettings* Settings = UZombieAudioSettings::GetOrLoadDefault();
	switch (NewState)
	{
	case EZombieMusicState::Menu:			PrimaryMusic = StartLoop(Settings->MenuMusic); break;
	case EZombieMusicState::Boss:			PrimaryMusic = StartLoop(Settings->BossMusic); break;
	case EZombieMusicState::Intermission:	PrimaryMusic = StartLoop(Settings->IntermissionMusic); break;
	case EZombieMusicState::GameOver:		PrimaryMusic = StartLoop(Settings->GameOverMusic); break;
	case EZombieMusicState::Sector:
		PrimaryMusic = StartLoop(Settings->ExplorationLayer);
		CombatMusic = StartLoop(Settings->CombatLayer);
		break;
	default:
		break;
	}

	if (NewState == EZombieMusicState::Sector || NewState == EZombieMusicState::Boss)
	{
		Ambience = StartLoop(Settings->AmbientLoop);
	}

	ApplyVolumeSettings();
}

float UZombieAudioSubsystem::MeasureThreat() const
{
	const UWorld* World = GetWorld();
	const AZombieGameState* GameState = World ? World->GetGameState<AZombieGameState>() : nullptr;
	if (!GameState)
	{
		return 0.0f;
	}

	const UZombieAudioSettings* Settings = UZombieAudioSettings::GetOrLoadDefault();
	const float RadiusSquared = FMath::Square(Settings->IntensityRadius);

	TArray<FVector> PlayerLocations;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APawn* Pawn = It->IsValid() ? It->Get()->GetPawn() : nullptr)
		{
			PlayerLocations.Add(Pawn->GetActorLocation());
		}
	}

	int32 NearbyZombies = 0;
	for (const AActor* Zombie : GameState->GetActiveZombies())
	{
		if (!IsValid(Zombie))
		{
			continue;
		}
		for (const FVector& PlayerLocation : PlayerLocations)
		{
			if (FVector::DistSquared2D(Zombie->GetActorLocation(), PlayerLocation) <= RadiusSquared)
			{
				++NearbyZombies;
				break;
			}
		}
	}

	return FMath::Clamp(NearbyZombies / FMath::Max(Settings->ZombiesForFullIntensity, 1.0f), 0.0f, 1.0f);
}

void UZombieAudioSubsystem::UpdateMix()
{
	if (MusicState != EZombieMusicState::Sector)
	{
		return;
	}

	const UZombieAudioSettings* Settings = UZombieAudioSettings::GetOrLoadDefault();
	const float Target = MeasureThreat();
	const float Rate = Target > CombatIntensity ? Settings->IntensityRiseRate : Settings->IntensityFallRate;
	CombatIntensity = FMath::FInterpConstantTo(CombatIntensity, Target, MixUpdateInterval, Rate);

	ApplyVolumeSettings();
}

void UZombieAudioSubsystem::ApplyVolumeSettings()
{
	const UZombieAudioSettings* Settings = UZombieAudioSettings::GetOrLoadDefault();
	const float MusicVolume = GetCategoryVolume(EZombieSoundCategory::Music);
	const float EffectsVolume = GetCategoryVolume(EZombieSoundCategory::Effects);

	if (IsValid(PrimaryMusic))
	{
		const FZombieSoundSpec& Spec = MusicState == EZombieMusicState::Sector ? Settings->ExplorationLayer
			: MusicState == EZombieMusicState::Boss ? Settings->BossMusic
			: MusicState == EZombieMusicState::Intermission ? Settings->IntermissionMusic
			: MusicState == EZombieMusicState::GameOver ? Settings->GameOverMusic
			: Settings->MenuMusic;

		// In a sector the exploration layer ducks as the combat layer swells.
		const float Duck = MusicState == EZombieMusicState::Sector ? FMath::Lerp(1.0f, 0.35f, CombatIntensity) : 1.0f;
		PrimaryMusic->SetVolumeMultiplier(FMath::Max(Spec.Volume * MusicVolume * Duck, 0.001f));
	}

	if (IsValid(CombatMusic))
	{
		CombatMusic->SetVolumeMultiplier(FMath::Max(Settings->CombatLayer.Volume * MusicVolume * CombatIntensity, 0.001f));
	}

	if (IsValid(Ambience))
	{
		Ambience->SetVolumeMultiplier(FMath::Max(Settings->AmbientLoop.Volume * EffectsVolume, 0.001f));
	}
}
