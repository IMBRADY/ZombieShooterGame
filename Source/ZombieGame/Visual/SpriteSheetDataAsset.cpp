#include "SpriteSheetDataAsset.h"

FSpriteAnimation USpriteSheetDataAsset::FindAnimation(FName Name) const
{
	if (const FSpriteAnimation* Found = Animations.Find(Name))
	{
		return *Found;
	}

	FSpriteAnimation Still;
	Still.bLoop = false;
	return Still;
}
