#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>

namespace EmuEx
{

static void stampDuoElement(VControllerElement &elem, int16_t sizeMM100x, uint8_t playerSet, uint8_t role, uint8_t alpha)
{
	elem.sizeMM100x = sizeMM100x;
	elem.ownAlpha = alpha;
	elem.duoRole = role;
	elem.duoFlags = VControllerElement::DUO_POS_MM | VControllerElement::DUO_FIXED_PLAYER
		| uint8_t(playerSet << VControllerElement::DUO_PLAYER_SHIFT);
	if(alpha)
		elem.duoFlags |= VControllerElement::DUO_OWN_ALPHA;
	elem.layoutPos[0] = {LB2DO, {-1, -1}};
	elem.layoutPos[1] = {LB2DO, {-1, -1}};
}

VControllerElement &VController::addDuoButton(KeyInfo key, int16_t sizeMM100x, uint8_t playerSet, uint8_t role)
{
	VControllerButtonGroup::Config conf;
	conf.keys = {key};
	conf.layout.rowItems = 1;
	conf.layout.origin = LB2DO;
	auto &elem = gpElements.emplace_back(std::in_place_type<VControllerButtonGroup>, conf);
	uint8_t alpha = playerSet == 1 ? p2Alpha_ : playerSet == 0 ? p1Alpha_ : 0;
	stampDuoElement(elem, sizeMM100x, playerSet, role, alpha);
	if(hasWindow())
		update(elem);
	return elem;
}

VControllerElement &VController::addDuoDPad(std::span<const KeyInfo, 4> keys, int16_t sizeMM100x, uint8_t playerSet, uint8_t role)
{
	auto &elem = gpElements.emplace_back(std::in_place_type<VControllerDPad>, keys);
	uint8_t alpha = playerSet == 1 ? p2Alpha_ : playerSet == 0 ? p1Alpha_ : 0;
	stampDuoElement(elem, sizeMM100x, playerSet, role, alpha);
	if(hasWindow())
	{
		update(elem);
		if(elem.dPad())
			elem.dPad()->updateBoundingAreaGfx(renderer());
	}
	return elem;
}

void VController::setDuoPosition(VControllerElement &elem, int layoutIdx, _2DOrigin origin, float xMm, float yMm)
{
	elem.duoFlags |= VControllerElement::DUO_POS_MM;
	elem.layoutPos[layoutIdx].origin = origin;
	elem.layoutPos[layoutIdx].pos = {int16_t(std::lround(xMm * 10.f)), int16_t(std::lround(yMm * 10.f))};
}

void VController::setTwoPlayerControls(bool on)
{
	if(twoPlayer_ == on)
		return;
	twoPlayer_ = on;
	duoCustomized_ = false;
	resetEmulatedDeviceGroups();
	if(hasWindow())
	{
		place();
		app().viewController().placeEmuViews();
	}
}

void VController::setDuoLayout(DuoLayout layout)
{
	if(duoLayout_ == layout)
		return;
	duoLayout_ = layout;
	duoCustomized_ = false;
	if(twoPlayer_)
		resetEmulatedDeviceGroups();
	if(hasWindow())
	{
		place();
		app().viewController().placeEmuViews();
	}
}

void VController::setShowSharedFaceRow(bool on)
{
	if(showSharedRow_ == on)
		return;
	showSharedRow_ = on;
	duoCustomized_ = false;
	if(twoPlayer_)
		resetEmulatedDeviceGroups();
	if(hasWindow())
	{
		place();
		app().viewController().placeEmuViews();
	}
}

void VController::setPlayerFaceSizeMM100x(int player, int mm100x)
{
	mm100x = std::clamp(mm100x, 900, 3000);
	if(player)
		p2Size_ = int16_t(mm100x);
	else
		p1Size_ = int16_t(mm100x);
	duoCustomized_ = false;
	if(hasWindow())
		place();
}

void VController::setPlayerOpacity(int player, uint8_t alpha)
{
	if(player)
		p2Alpha_ = alpha;
	else
		p1Alpha_ = alpha;
	for(auto &elem : gpElements)
	{
		if(elem.playerSet() != (player ? 1 : 0))
			continue;
		elem.ownAlpha = alpha;
		elem.duoFlags |= VControllerElement::DUO_OWN_ALPHA;
	}
	applySavedButtonAlpha();
}

int VController::activeGamepadLayoutIndex() const
{
	if(!twoPlayer_ || !hasWindow())
		return hasWindow() ? int(window().isPortrait()) : 0;
	float aspect = float(window().width()) / float(std::max(1, window().height()));
	return aspect > 1.6f ? 0 : 1;
}

int VController::videoBottomInsetPx() const
{
	if(!twoPlayer_ || !hasWindow() || activeGamepadLayoutIndex() == 0)
		return 0;
	return std::max(0, yMMSizeToPixel(window(), bottomInsetMM_));
}

void VController::noteDuoDrag()
{
	duoCustomized_ = true;
}

void VController::rebuildDuoIfNeeded()
{
	if(twoPlayer_)
		resetEmulatedDeviceGroups();
}

}
