#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/AppKeyCode.hh>

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
	uint8_t alpha = playerSet == 1 ? p2Alpha_ : playerSet == 2 ? sharedAlpha_ : p1Alpha_;
	stampDuoElement(elem, sizeMM100x, playerSet, role, alpha);
	if(hasWindow())
		update(elem);
	return elem;
}

VControllerElement &VController::addDuoDPad(std::span<const KeyInfo, 4> keys, int16_t sizeMM100x, uint8_t playerSet, uint8_t role)
{
	auto &elem = gpElements.emplace_back(std::in_place_type<VControllerDPad>, keys);
	uint8_t alpha = playerSet == 1 ? p2Alpha_ : playerSet == 2 ? sharedAlpha_ : p1Alpha_;
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
	syncDuoModeButton();
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

static void applyOpacity(VControllerElement &elem, uint8_t alpha)
{
	elem.ownAlpha = alpha;
	if(alpha)
		elem.duoFlags |= VControllerElement::DUO_OWN_ALPHA;
}

void VController::setPlayerFaceSizeMM100x(int player, int mm100x)
{
	mm100x = std::clamp(mm100x, 900, 3000);
	if(lockPlayerSizes_)
		p1Size_ = p2Size_ = int16_t(mm100x);
	else if(player)
		p2Size_ = int16_t(mm100x);
	else
		p1Size_ = int16_t(mm100x);
	// A size change keeps dragged anchors. The default layout still rebuilds
	// when the player has not moved anything.
	if(duoCustomized_)
		AppMeta::refreshDuoButtonSizes(*this);
	if(hasWindow())
		place();
}

void VController::setSharedFaceSizeMM100x(int mm100x)
{
	sharedSize_ = int16_t(std::clamp(mm100x, 900, 3000));
	if(duoCustomized_)
		AppMeta::refreshDuoButtonSizes(*this);
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
		applyOpacity(elem, alpha);
	}
	applySavedButtonAlpha();
}

void VController::setSharedOpacity(uint8_t alpha)
{
	sharedAlpha_ = alpha;
	for(auto &elem : gpElements)
	{
		if(elem.playerSet() != 2)
			continue;
		applyOpacity(elem, alpha);
	}
	applySavedButtonAlpha();
}

void VController::setLockPlayerSizes(bool on)
{
	lockPlayerSizes_ = on;
	if(on)
		setPlayerFaceSizeMM100x(0, p1Size_);
}

void VController::setVideoBottomInsetMM(int layout, float mm)
{
	if(layout >= 0 && layout < 2)
		bottomInsetMM_[layout] = mm;
}

int VController::activeGamepadLayoutIndex() const
{
	if(!hasWindow())
		return 0;
	return int(window().isPortrait());
}

int VController::videoBottomInsetPx() const
{
	if(!twoPlayer_ || !hasWindow())
		return 0;
	int idx = activeGamepadLayoutIndex();
	return std::max(0, yMMSizeToPixel(window(), bottomInsetMM_[idx]));
}

void VController::noteDuoDrag()
{
	duoCustomized_ = true;
}

void VController::syncDuoModeButton()
{
	std::erase_if(uiElements, [](const VControllerElement &elem)
	{
		auto *grp = elem.uiButtonGroup();
		if(!grp)
			return false;
		return std::ranges::any_of(grp->buttons, [](const VControllerButton &btn)
		{
			return btn.key.flags.appCode && AppKeyCode(btn.key.codes[0]) == AppKeyCode::toggleDuoLayout;
		});
	});
	if(!twoPlayer_)
		return;
	VControllerUIButtonGroup::Config conf;
	conf.keys = {KeyInfo::appKey(AppKeyCode::toggleDuoLayout)};
	conf.layout.rowItems = 1;
	conf.layout.origin = CT2DO;
	auto &elem = uiElements.emplace_back(std::in_place_type<VControllerUIButtonGroup>, conf);
	elem.sizeMM100x = 900;
	elem.duoFlags = VControllerElement::DUO_POS_MM;
	// Select/Start sit at about 9.5 mm from the top. 30 mm puts this button
	// about 20 mm below them so the 9 mm discs do not overlap.
	setDuoPosition(elem, 0, CT2DO, 0.f, 30.f);
	setDuoPosition(elem, 1, CT2DO, 0.f, 30.f);
	if(hasWindow())
		update(elem);
}

void VController::rebuildDuoIfNeeded()
{
	if(twoPlayer_)
		resetEmulatedDeviceGroups();
}

}
