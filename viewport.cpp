#include "viewport.hpp"
#include "gfx.hpp"
#include "game.hpp"
#include "text.hpp"
#include "math.hpp"
#include "constants.hpp"

#include <iostream>

struct PreserveClipRect
{
	PreserveClipRect(SDL_Surface* surf)
	: surf(surf)
	{
		SDL_GetClipRect(surf, &rect);
	}
	
	~PreserveClipRect()
	{
		SDL_SetClipRect(surf, &rect);
	}
	
	SDL_Surface* surf;
	SDL_Rect rect;
};

void Viewport::process()
{
	Common& common = *game.common;
	if(worm->killedTimer <= 0)
	{
		if(worm->visible)
		{
			int sumX = 0;
			int sumY = 0;
			
			int objectsFound = 0;
			
			WormWeapon const& ww = worm->weapons[worm->currentWeapon];
			
			if(common.weapons[ww.id].shotType == Weapon::STSteerable)
			{
				for(Game::WObjectList::iterator i = game.wobjects.begin(); i != game.wobjects.end(); ++i)
				{
					if(i->id == ww.id && i->owner == worm)
					{
						++objectsFound;
						sumX += ftoi(i->x);
						sumY += ftoi(i->y);
					}
				}
			}
			
			if(objectsFound > 0)
			{
				setCenter(sumX / objectsFound, sumY / objectsFound);
			}
			else
			{
				setCenter(ftoi(worm->x), ftoi(worm->y));
			}
		}
		else
		{
			scrollTo(ftoi(worm->x), ftoi(worm->y), 4);
		}
	}
	else if(worm->health < 0)
	{
		setCenter(ftoi(worm->x), ftoi(worm->y));
		
		if(worm->killedTimer == 150) // TODO: This depends on what is the starting killedTimer
			bannerY = -8;
	}
	
	int realShake = ftoi(shake);
	
	if(realShake > 0)
	{
		x += rand(realShake * 2) - realShake;
		y += rand(realShake * 2) - realShake;
	}
	
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x > maxX) x = maxX;
	if(y > maxY) y = maxY;
	
	/*
	if(worm->health <= 0)
	{
		if((game.cycles & 1) == 0)
		{
			if(worm->killedTimer > 16)
			{
				if(bannerY < 2)
					++bannerY;
			}
			else
			{
				if(bannerY > -8)
					--bannerY;
			}
		}
	}*/
}

void Viewport::draw(bool isReplay)
{
	Common& common = *game.common;
	if(worm) // Should not be necessary further on
	{
		if(worm->visible)
		{
			int lifebarWidth = worm->health * 100 / worm->settings->health;
			drawBar(inGameX, 161, lifebarWidth, lifebarWidth/10 + 234);
		}
		else
		{
			int lifebarWidth = 100 - (worm->killedTimer * 25) / 37;
			if(lifebarWidth > 0)
			{
				if(lifebarWidth > 100)
					lifebarWidth = 100;
				drawBar(inGameX, 161, lifebarWidth, lifebarWidth/10 + 234);
			}
		}
	}
	
	// Draw kills status
	
	WormWeapon const& ww = worm->weapons[worm->currentWeapon];
	
	if(ww.available)
	{
		if(ww.ammo > 0)
		{
			int ammoBarWidth = ww.ammo * 100 / common.weapons[ww.id].ammo;
			
			if(ammoBarWidth > 0)
				drawBar(inGameX, 166, ammoBarWidth, ammoBarWidth/10 + 245);
		}
	}
	else
	{
		int ammoBarWidth = 0;
		
		if(common.weapons[ww.id].loadingTime != 0)
		{
			int computedLoadingTime = common.weapons[ww.id].computedLoadingTime(*game.settings);
			ammoBarWidth = 100 - ww.loadingLeft * 100 / computedLoadingTime;
		}
		else
		{
			ammoBarWidth = 100 - ww.loadingLeft * 100;
		}
		
		if(ammoBarWidth > 0)
			drawBar(inGameX, 166, ammoBarWidth, ammoBarWidth/10 + 245);
		
		if((game.cycles % 20) > 10
		&& worm->visible)
		{
			common.font.drawText(common.texts.reloading, inGameX, 164, 50);
		}
	}
	
	common.font.drawText((common.texts.kills + toString(worm->kills)), inGameX, 171, 10);
	
	if(isReplay)
	{
		common.font.drawText(worm->settings->name, inGameX, 192, 4);
		common.font.drawText(timeToStringEx(game.cycles * 14), 95, 185, 7);
	}
	
	switch(game.settings->gameMode)
	{
	case Settings::GMKillEmAll:
	{
		common.font.drawText((common.texts.lives + toString(worm->lives)), inGameX, 178, 6);
	}
	break;
	
	case Settings::GMGameOfTag:
	{
		int const stateColours[] = {6, 10, 79, 4};
		
		int state = 0;
		
		for(std::size_t i = 0; i < game.worms.size(); ++i)
		{
			Worm& w = *game.worms[i];
			
			if(&w != worm
			&& w.timer >= worm->timer)
				state = 1; // We're better or equal off
		}
		
		int color;
		if(game.lastKilled == worm)
			color = stateColours[state];
		else
			color = stateColours[state + 2];
		
		common.font.drawText(timeToString(worm->timer), 5, 106 + 84*worm->index, 161, color);
	}
	break;
	}	

	PreserveClipRect pcr(gfx.screen);
	
	SDL_Rect viewportClip;
	viewportClip.x = rect.x1;
	viewportClip.y = rect.y1;
	viewportClip.w = rect.width();
	viewportClip.h = rect.height();
	
	SDL_SetClipRect(gfx.screen, &viewportClip);
	
	int offsX = rect.x1 - x;
	int offsY = rect.y1 - y;
	
	blitImageNoKeyColour(gfx.screen, &game.level.data[0], offsX, offsY, game.level.width, game.level.height); // TODO: Unhardcode
	
	if(!worm->visible
	&& worm->killedTimer <= 0
	&& !worm->ready)
	{
		common.font.drawText(common.texts.pressFire, rect.center_x() - 30, 76, 0);
		common.font.drawText(common.texts.pressFire, rect.center_x() - 31, 75, 50);
	}

	if(bannerY > -8
	&& worm->health <= 0)
	{	
		if(game.settings->gameMode == Settings::GMGameOfTag
		&& game.gotChanged)
		{
			common.font.drawText(common.S[YoureIt], rect.x1 + 3, bannerY + 1, 0);
			common.font.drawText(common.S[YoureIt], rect.x1 + 2, bannerY, 50);
		}
	}
	
	for(std::size_t i = 0; i < game.viewports.size(); ++i)
	{
		Viewport* v = game.viewports[i];
		if(v != this
		&& v->worm->health <= 0
		&& v->bannerY > -8)
		{
			if(v->worm->lastKilledBy == worm)
			{
				std::string msg(common.S[KilledMsg] + v->worm->settings->name);
				common.font.drawText(msg, rect.x1 + 3, v->bannerY + 1, 0);
				common.font.drawText(msg, rect.x1 + 2, v->bannerY, 50);
			}
			else
			{
				std::string msg(v->worm->settings->name + common.S[CommittedSuicideMsg]);
				common.font.drawText(msg, rect.x1 + 3, v->bannerY + 1, 0);
				common.font.drawText(msg, rect.x1 + 2, v->bannerY, 50);
			}
		}
	}

	for(Game::BonusList::iterator i = game.bonuses.begin(); i != game.bonuses.end(); ++i)
	{
		if(i->timer > common.C[BonusFlickerTime] || (game.cycles & 3) == 0)
		{
			int f = common.bonusFrames[i->frame];
			
			blitImage(
				gfx.screen,
				common.smallSprites.spritePtr(f),
				ftoi(i->x) - x - 3 + rect.x1,
				ftoi(i->y) - y - 3 + rect.y1,
				7, 7);
				
			if(game.settings->shadow)
			{
				blitShadowImage(
					common,
					gfx.screen,
					common.smallSprites.spritePtr(f),
					ftoi(i->x) - x - 5 + rect.x1,
					ftoi(i->y) - y - 1 + rect.y1, // This was - 3 in the original, but that seems wrong
					7, 7);
			}
			
			if(game.settings->namesOnBonuses
			&& i->frame == 0)
			{
				std::string const& name = common.weapons[i->weapon].name;
				int len = int(name.size()) * 4;
				
				common.drawTextSmall(
					name.c_str(),
					ftoi(i->x) - x - len/2 + rect.x1,
					ftoi(i->y) - y - 10 + rect.y1);
			}
		}
	}
		
	for(Game::SObjectList::iterator i = game.sobjects.begin(); i != game.sobjects.end(); ++i)
	{
		SObjectType const& t = common.sobjectTypes[i->id];
		int frame = i->curFrame + t.startFrame;
		
		// TODO: Check that blitImageR is the correct one to use (probably)
		blitImageR(
			gfx.screen,
			common.largeSprites.spritePtr(frame),
			i->x + offsX,
			i->y + offsY,
			16, 16);
			
		if(game.settings->shadow)
		{
			blitShadowImage(
				common,
				gfx.screen,
				common.largeSprites.spritePtr(frame),
				i->x + offsX - 3,
				i->y + offsY + 3, // TODO: Original doesn't offset the shadow, which is clearly wrong. Check that this offset is correct.
				16, 16);
		}
	}
		
	// TODO: Check order of drawing between bonuses, wobjects, etc.
	
	for(Game::WObjectList::iterator i = game.wobjects.begin(); i != game.wobjects.end(); ++i)
	{
		Weapon const& w = common.weapons[i->id];
		
		if(w.startFrame > -1)
		{
			int curFrame = i->curFrame;
			int shotType = w.shotType;
			
			if(shotType == 2)
			{
				curFrame += 4;
				curFrame >>= 3;
				if(curFrame < 0)
					curFrame = 16;
				else if(curFrame > 15)
					curFrame -= 16;
			}
			else if(shotType == 3)
			{
				if(curFrame > 64)
					--curFrame;
				curFrame -= 12;
				curFrame >>= 3;
				if(curFrame < 0)
					curFrame = 0;
				else if(curFrame > 12)
					curFrame = 12;
			}
			
			int posX = ftoi(i->x) - 3;
			int posY = ftoi(i->y) - 3;
			
			if(game.settings->shadow
			&& w.shadow)
			{
				blitShadowImage(
					common,
					gfx.screen,
					common.smallSprites.spritePtr(w.startFrame + curFrame),
					posX - x - 3 + rect.x1,
					posY - y + 3 + rect.y1, // TODO: Combine rect.x1 - x into one number, same with y
					7,
					7);
			}
			
			blitImage(
				gfx.screen,
				common.smallSprites.spritePtr(w.startFrame + curFrame),
				posX - x + rect.x1,
				posY - y + rect.y1, // TODO: Combine rect.x1 - x into one number, same with y
				7,
				7);
		}
		else if(i->curFrame > 0)
		{
			int posX = ftoi(i->x) - x + rect.x1;
			int posY = ftoi(i->y) - y + rect.y1;
			
			if(isInside(gfx.screen->clip_rect, posX, posY))
				gfx.screenPixels[posY*gfx.screenPitch + posX] = static_cast<PalIdx>(i->curFrame);
			
			if(game.settings->shadow)
			{
				posX -= 3;
				posY += 3;
				
				if(isInside(gfx.screen->clip_rect, posX, posY))
				{
					PalIdx& pix = gfx.screenPixels[posY*gfx.screenPitch + posX];
					if(common.materials[pix].seeShadow())
						pix += 4;
				}
			}
			
		}
		
		if(!common.H[HRemExp] && i->id == 34 && game.settings->namesOnBonuses) // TODO: Read from EXE
		{
			if(i->curFrame == 0)
			{
				int nameNum = int(&*i - game.wobjects.arr) % 40; // TODO: Something nicer maybe
				
				std::string const& name = common.weapons[nameNum].name;
				int width = int(name.size()) * 4;
				
				common.drawTextSmall(
					name.c_str(),
					ftoi(i->x) - x - width/2 + rect.x1,
					ftoi(i->y) - y - 10 + rect.y1);
			}
		}
	}
	
	for(Game::NObjectList::iterator i = game.nobjects.begin(); i != game.nobjects.end(); ++i)
	{
		NObjectType const& t = common.nobjectTypes[i->id];
		
		if(t.startFrame > 0)
		{
			int posX = ftoi(i->x) - 3;
			int posY = ftoi(i->y) - 3;
			
			if(i->id >= 20 && i->id <= 21)
			{
				// Flag special case
				posY -= 2;
				posX += 3;
			}
			
			if(game.settings->shadow)
			{
				blitShadowImage(
					common,
					gfx.screen,
					common.smallSprites.spritePtr(t.startFrame + i->curFrame),
					posX - 3 + offsX,
					posY + 3 + offsY,
					7,
					7);
			}
			
			blitImage(
				gfx.screen,
				common.smallSprites.spritePtr(t.startFrame + i->curFrame),
				posX + offsX,
				posY + offsY,
				7,
				7);
				
		}
		else if(i->curFrame > 1)
		{
			int posX = ftoi(i->x) + offsX;
			int posY = ftoi(i->y) + offsY;
			if(isInside(gfx.screen->clip_rect, posX, posY))
				gfx.getScreenPixel(posX, posY) = PalIdx(i->curFrame);
				
			if(game.settings->shadow)
			{
				posX -= 3;
				posY += 3;
				
				if(isInside(gfx.screen->clip_rect, posX, posY))
				{
					PalIdx& pix = gfx.getScreenPixel(posX, posY);
					if(common.materials[pix].seeShadow())
						pix += 4;
				}
			}
		}
	}

	for(std::size_t i = 0; i < game.worms.size(); ++i)
	{
		Worm const& w = *game.worms[i];
		
		if(w.visible)
		{
			
			int tempX = ftoi(w.x) - x - 7 + rect.x1;
			int tempY = ftoi(w.y) - y - 5 + rect.y1;
			int angleFrame = w.angleFrame();
			
			if(w.weapons[w.currentWeapon].available)
			{
				int hotspotX = w.hotspotX - x + rect.x1;
				int hotspotY = w.hotspotY - y + rect.y1;
				
				WormWeapon const& ww = w.weapons[w.currentWeapon];
				Weapon& weapon = common.weapons[ww.id];
				
				if(weapon.laserSight)
				{
					drawLaserSight(hotspotX, hotspotY, tempX + 7, tempY + 4);
				}
				
				if(ww.id == common.C[LaserWeapon] - 1 && w.pressed(Worm::Fire))
				{
					drawLine(hotspotX, hotspotY, tempX + 7, tempY + 4, weapon.colorBullets);
				}
			}
			
			if(w.ninjarope.out)
			{
				int ninjaropeX = ftoi(w.ninjarope.x) - x + rect.x1;
				int ninjaropeY = ftoi(w.ninjarope.y) - y + rect.y1;
				
				drawNinjarope(common, ninjaropeX, ninjaropeY, tempX + 7, tempY + 4);
				
				blitImage(gfx.screen, common.largeSprites.spritePtr(84), ninjaropeX - 1, ninjaropeY - 1, 16, 16);
				
				if(game.settings->shadow)
				{
					drawShadowLine(common, ninjaropeX - 3, ninjaropeY + 3, tempX + 7 - 3, tempY + 4 + 3);
					blitShadowImage(common, gfx.screen, common.largeSprites.spritePtr(84), ninjaropeX - 4, ninjaropeY + 2, 16, 16);
				}
				
			}
			
			if(common.weapons[w.weapons[w.currentWeapon].id].fireCone > -1
			&& w.fireConeActive)
			{
				/* TODO
				//NOTE! Check fctab so it's correct
				//NOTE! Check function 1071C and see what it actually does*/
				
				blitFireCone(
					gfx.screen,
					w.fireCone / 2,
					common.fireConeSprite(angleFrame, w.direction),
					common.fireConeOffset[w.direction][angleFrame][0] + tempX,
					common.fireConeOffset[w.direction][angleFrame][1] + tempY);
			}
			
			
			blitImage(gfx.screen, common.wormSprite(w.currentFrame, w.direction, w.index), tempX, tempY, 16, 16);
			if(game.settings->shadow)
				blitShadowImage(common, gfx.screen, common.wormSprite(w.currentFrame, w.direction, w.index), tempX - 3, tempY + 3, 16, 16);
		}
	}
	
	if(worm->visible)
	{
		int tempX = ftoi(worm->x) - x - 1 + ftoi(cosTable[ftoi(worm->aimingAngle)] * 16) + rect.x1;
		int tempY = ftoi(worm->y) - y - 2 + ftoi(sinTable[ftoi(worm->aimingAngle)] * 16) + rect.y1;
		
		if(worm->makeSightGreen)
		{
			blitImage(
				gfx.screen,
				common.smallSprites.spritePtr(44),
				tempX,
				tempY,
				7, 7);
		}
		else
		{
			blitImage(
				gfx.screen,
				common.smallSprites.spritePtr(43),
				tempX,
				tempY,
				7, 7);
		}
		
#ifdef TEMP
		common.font.drawText(toString(worm->reacts[0]), 10 + rect.x1, 10, 10);
		common.font.drawText(toString(worm->reacts[1]), 20 + rect.x1, 10, 10);
		common.font.drawText(toString(worm->reacts[2]), 30 + rect.x1, 10, 10);
		common.font.drawText(toString(worm->reacts[3]), 40 + rect.x1, 10, 10);
		
		if(ftoi(worm->x) < 4 && worm->velX < 0 && worm->reacts[Worm::RFRight] < 2)
		{
			std::cout << worm->reacts[Worm::RFRight] << ", " << worm->velX << ", " << worm->x << std::endl;
			common.font.drawText(":O", 50 + rect.x1, 10, 10);
		}
#endif
		if(worm->pressed(Worm::Change))
		{
			int id = worm->weapons[worm->currentWeapon].id;
			std::string const& name = common.weapons[id].name;
			
			int len = int(name.size()) * 4; // TODO: Read 4 from exe? (SW_CHARWID)
			
			common.drawTextSmall(
				name.c_str(),
				ftoi(worm->x) - x - len/2 + 1 + rect.x1,
				ftoi(worm->y) - y - 10 + rect.y1);
		}
	}
	
	for(Game::BObjectList::iterator i = game.bobjects.begin(); i != game.bobjects.end(); ++i)
	{
		int posX = ftoi(i->x) + offsX;
		int posY = ftoi(i->y) + offsY;
		if(isInside(gfx.screen->clip_rect, posX, posY))
			gfx.getScreenPixel(posX, posY) = PalIdx(i->color);
			
		if(game.settings->shadow)
		{
			posX -= 3;
			posY += 3;
			
			if(isInside(gfx.screen->clip_rect, posX, posY))
			{
				PalIdx& pix = gfx.getScreenPixel(posX, posY);
				if(common.materials[pix].seeShadow())
					pix += 4;
			}
		}
	}
	
	if(game.settings->map)
	{
		int my = 5;
		for(int y = 162; y < 197; ++y)
		{
			int mx = 5;
			for(int x = 134; x < 185; ++x)
			{
				gfx.getScreenPixel(x, y) = game.level.checkedPixelWrap(mx, my);
				mx += 10;
			}
			my += 10;
		}
		
		for(std::size_t i = 0; i < game.worms.size(); ++i)
		{
			Worm const& w = *game.worms[i];
			
			if(w.visible)
			{
				int x = ftoi(w.x) / 10 + 134;
				int y = ftoi(w.y) / 10 + 162;
				
				gfx.getScreenPixel(x, y) = 129 + w.index * 4;
			}
		}
	}
}
