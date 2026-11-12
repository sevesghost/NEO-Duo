/*  This file is part of NEO-Duo.

	NEO-Duo is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version. */

#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
import system;
import imagine;

namespace EmuEx
{

using namespace IG;

namespace
{

enum Role : uint8_t
{
	P1Dpad, P1A, P1B, P1C, P1D,
	P1TA, P1TB, P1TC, P1TD,
	P1AB, P1CD, P1ABC,
	P1TAB, P1TCD, P1TABC,
	P1Coin, P1Start,
	P2Base = 32,
	P2Dpad = P2Base, P2A, P2B, P2C, P2D,
	P2TA, P2TB, P2TC, P2TD,
	P2AB, P2CD, P2ABC,
	P2TAB, P2TCD, P2TABC,
	P2Coin, P2Start,
	BothA = 64, BothB, BothC, BothD
};

KeyInfo faceKey(NeoKey code, int player, bool both, bool turbo)
{
	KeyInfo key{code};
	key.flags.deviceId = both ? 0 : uint8_t(player);
	key.flags.bothPlayers = both;
	key.flags.turbo = turbo;
	return key;
}

KeyInfo combo2(NeoKey a, NeoKey b, int player, bool both, bool turbo)
{
	KeyInfo key{std::array{a, b}};
	key.flags.deviceId = both ? 0 : uint8_t(player);
	key.flags.bothPlayers = both;
	key.flags.turbo = turbo;
	return key;
}

KeyInfo combo3(NeoKey a, NeoKey b, NeoKey c, int player, bool both, bool turbo)
{
	KeyInfo key{std::array{a, b, c}};
	key.flags.deviceId = both ? 0 : uint8_t(player);
	key.flags.bothPlayers = both;
	key.flags.turbo = turbo;
	return key;
}

std::array<KeyInfo, 4> dpadKeys(int player, bool both)
{
	return {faceKey(NeoKey::Up, player, both, false), faceKey(NeoKey::Right, player, both, false),
		faceKey(NeoKey::Down, player, both, false), faceKey(NeoKey::Left, player, both, false)};
}

float canvasWidthMM(const Window &win)
{
	return win.width() / float(std::max(1, win.widthMMInPixels(1.f)));
}
float canvasHeightMM(const Window &win)
{
	return win.height() / float(std::max(1, win.heightMMInPixels(1.f)));
}

void put(VController &v, VControllerElement &elem, int layout, float canvasW, float canvasH, float xFromLeft, float yFromTop)
{
	// LB origin: +x moves right from the left edge, negative y moves up from the bottom.
	(void)canvasW;
	v.setDuoPosition(elem, layout, LB2DO, xFromLeft, yFromTop - canvasH);
}

float fitScale(float b1, float b2, float canvasW, bool joint)
{
	auto needed = [&](float scale)
	{
		constexpr float margin = 5.f, gap = 2.f;
		float B1 = b1 * scale, B2 = joint ? B1 : b2 * scale;
		if(joint)
		{
			float left = margin + B1 * 2.f;
			float right = margin + 4.f * B1 + 3.f * gap;
			return left + right + gap;
		}
		auto cluster = [](float b, float gap) { return b * 2.f + gap + 4.f * b + 3.f * gap; };
		return margin + cluster(B1, gap) + gap + cluster(B2, gap) + margin;
	};
	float scale = 1.f;
	if(needed(1.f) <= canvasW)
		return 1.f;
	while(scale > 0.45f && needed(scale) > canvasW)
		scale -= 0.05f;
	return scale;
}

void layoutCanvas(VController &v, int layout, float W, float H, bool wide)
{
	const bool joint = v.duoLayout() == DuoLayout::Joint;
	const bool shared = !joint && v.showSharedFaceRow();
	constexpr float margin = 5.f, gap = 2.f;
	float b1 = std::max(9.f, v.playerFaceSizeMM100x(0) / 100.f);
	float b2 = std::max(9.f, v.playerFaceSizeMM100x(1) / 100.f);
	float scale = fitScale(b1, b2, W, joint);
	b1 *= scale;
	b2 *= scale;
	b1 = std::max(b1, 7.f);
	b2 = std::max(b2, 7.f);
	auto faceOf = [&](const VControllerElement &elem)
	{
		if(elem.playerSet() == 1)
			return b2;
		return b1;
	};
	for(auto &elem : v.deviceElements())
	{
		if(elem.duoRole == 255)
			continue;
		float face = faceOf(elem);
		bool dpad = elem.dPad() != nullptr;
		bool coin = elem.duoRole == P1Coin || elem.duoRole == P1Start || elem.duoRole == P2Coin || elem.duoRole == P2Start;
		float size = coin ? std::min(9.f, face) : dpad ? face * 2.f : face;
		elem.sizeMM100x = int16_t(std::lround(size * 100.f));
	}

	auto findRole = [&](uint8_t role) -> VControllerElement *
	{
		for(auto &elem : v.deviceElements())
			if(elem.duoRole == role)
				return &elem;
		return nullptr;
	};
	auto placeRole = [&](uint8_t role, float x, float y)
	{
		if(auto *elem = findRole(role))
			put(v, *elem, layout, W, H, x, y);
	};

	float mid = H * 0.5f;
	float row = 0.f;
	auto placeCluster = [&](int baseRole, float leftX, float dpadSize, float btn, float bottomY, bool fromBottom)
	{
		float dpadX = leftX + dpadSize * 0.5f;
		float dpadY = fromBottom ? bottomY + btn * 0.5f - dpadSize * 0.5f : mid;
		placeRole(baseRole + P1Dpad, dpadX, dpadY);
		float buttonsLeft = leftX + dpadSize + gap;
		float adY = fromBottom ? bottomY : mid + (btn + gap);
		float turboY = adY - btn - gap;
		float comboY = turboY - btn - gap;
		row = btn + gap;
		for(int i = 0; i < 4; i++)
		{
			float x = buttonsLeft + btn * 0.5f + i * (btn + gap);
			placeRole(baseRole + P1A + i, x, adY);
			placeRole(baseRole + P1TA + i, x, turboY);
		}
		float faceW = 4.f * btn + 3.f * gap;
		float comboW = 3.f * btn + 2.f * gap;
		float comboLeft = buttonsLeft + (faceW - comboW) * 0.5f;
		placeRole(baseRole + P1AB, comboLeft + btn * 0.5f, comboY);
		placeRole(baseRole + P1CD, comboLeft + btn * 0.5f + btn + gap, comboY);
		placeRole(baseRole + P1ABC, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY);
		placeRole(baseRole + P1TAB, comboLeft + btn * 0.5f, comboY - btn - gap);
		placeRole(baseRole + P1TCD, comboLeft + btn * 0.5f + btn + gap, comboY - btn - gap);
		placeRole(baseRole + P1TABC, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY - btn - gap);
		float coin = std::min(9.f, btn);
		float coinY = fromBottom
			? std::min(comboY - btn - gap, dpadY - dpadSize * 0.5f) - gap - coin * 0.5f
			: dpadY + dpadSize * 0.5f + gap + coin * 0.5f;
		if(!fromBottom && coinY + coin * 0.5f > H - margin)
			coinY = dpadY - dpadSize * 0.5f - gap - coin * 0.5f;
		float coinX = leftX + coin * 0.5f;
		// P2's cluster is mirrored, so its "left" is actually the right edge of the cluster.
		bool mirrored = baseRole == P2Base;
		if(mirrored)
		{
			coinX = leftX + dpadSize + gap + faceW - coin * 0.5f;
			placeRole(P2Coin, coinX, coinY);
			placeRole(P2Start, coinX - coin - gap, coinY);
		}
		else
		{
			placeRole(P1Coin, coinX, coinY);
			placeRole(P1Start, coinX + coin + gap, coinY);
		}
		(void)row;
	};

	if(joint)
	{
		float btn = b1;
		float dpad = btn * 2.f;
		if(wide)
		{
			placeCluster(0, margin, dpad, btn, 0, false);
			// Move the button cluster to the right edge. placeCluster anchored it on the left;
			// re-place the face/turbo/combo rows on the right, leaving the dpad where it is.
			float faceW = 4.f * btn + 3.f * gap;
			float buttonsLeft = W - margin - faceW;
			float adY = mid + btn + gap;
			float turboY = mid;
			float comboY = mid - (btn + gap);
			for(int i = 0; i < 4; i++)
			{
				float x = buttonsLeft + btn * 0.5f + i * (btn + gap);
				placeRole(P1A + i, x, adY);
				placeRole(P1TA + i, x, turboY);
			}
			float comboW = 3.f * btn + 2.f * gap;
			float comboLeft = buttonsLeft + (faceW - comboW) * 0.5f;
			placeRole(P1AB, comboLeft + btn * 0.5f, comboY);
			placeRole(P1CD, comboLeft + btn * 0.5f + btn + gap, comboY);
			placeRole(P1ABC, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY);
			placeRole(P1TAB, comboLeft + btn * 0.5f, comboY - btn - gap);
			placeRole(P1TCD, comboLeft + btn * 0.5f + btn + gap, comboY - btn - gap);
			placeRole(P1TABC, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY - btn - gap);
			float coin = std::min(9.f, btn);
			placeRole(P1Coin, margin + coin * 0.5f, margin + coin * 0.5f);
			placeRole(P1Start, margin + coin * 1.5f + gap, margin + coin * 0.5f);
			placeRole(P2Coin, W - margin - coin * 0.5f, margin + coin * 0.5f);
			placeRole(P2Start, W - margin - coin * 1.5f - gap, margin + coin * 0.5f);
			v.setVideoBottomInsetMM(0);
		}
		else
		{
			float bottom = H - margin - btn * 0.5f;
			placeCluster(0, margin, dpad, btn, bottom, true);
			float faceW = 4.f * btn + 3.f * gap;
			float buttonsLeft = W - margin - faceW;
			float adY = bottom;
			float turboY = adY - btn - gap;
			float comboY = turboY - btn - gap;
			for(int i = 0; i < 4; i++)
			{
				float x = buttonsLeft + btn * 0.5f + i * (btn + gap);
				placeRole(P1A + i, x, adY);
				placeRole(P1TA + i, x, turboY);
			}
			float comboW = 3.f * btn + 2.f * gap;
			float comboLeft = buttonsLeft + (faceW - comboW) * 0.5f;
			placeRole(P1AB, comboLeft + btn * 0.5f, comboY);
			placeRole(P1CD, comboLeft + btn * 0.5f + btn + gap, comboY);
			placeRole(P1ABC, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY);
			placeRole(P1TAB, comboLeft + btn * 0.5f, comboY - btn - gap);
			placeRole(P1TCD, comboLeft + btn * 0.5f + btn + gap, comboY - btn - gap);
			placeRole(P1TABC, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY - btn - gap);
			float coin = std::min(9.f, btn);
			float coinY = comboY - (btn + gap) - gap - coin * 0.5f;
			placeRole(P1Coin, margin + coin * 0.5f, coinY);
			placeRole(P1Start, margin + coin * 1.5f + gap, coinY);
			placeRole(P2Coin, W - margin - coin * 0.5f, coinY);
			placeRole(P2Start, W - margin - coin * 1.5f - gap, coinY);
			float stripTop = coinY - coin * 0.5f - margin;
			v.setVideoBottomInsetMM(std::max(0.f, H - stripTop));
		}
		return;
	}

	// Separate: P1 on the left, P2 mirrored on the right.
	float bottom = wide ? 0.f : H - margin - (shared ? (b1 + gap + b1 * 0.5f) : b1 * 0.5f);
	float p1Left = margin;
	float p2RightEdge = W - margin;
	float d1 = b1 * 2.f, d2 = b2 * 2.f;
	float p2Left = p2RightEdge - (d2 + gap + 4.f * b2 + 3.f * gap);
	if(shared)
	{
		float total = 4.f * b1 + 3.f * gap;
		float x0 = (W - total) * 0.5f + b1 * 0.5f;
		float y = H - margin - b1 * 0.5f;
		placeRole(BothA, x0, y);
		placeRole(BothB, x0 + b1 + gap, y);
		placeRole(BothC, x0 + 2.f * (b1 + gap), y);
		placeRole(BothD, x0 + 3.f * (b1 + gap), y);
	}
	placeCluster(0, p1Left, d1, b1, bottom, !wide);
	// P2 is mirrored: dpad on the far right, buttons to its left.
	{
		float btn = b2;
		float dpad = d2;
		float faceW = 4.f * btn + 3.f * gap;
		float buttonsLeft = p2Left;
		float dpadX = p2RightEdge - dpad * 0.5f;
		float adY = wide ? mid + btn + gap : bottom;
		float dpadY = wide ? mid : adY + btn * 0.5f - dpad * 0.5f;
		placeRole(P2Dpad, dpadX, dpadY);
		float turboY = adY - btn - gap;
		float comboY = turboY - btn - gap;
		// A sits closest to the dpad, so the row runs D C B A from left to right.
		for(int i = 0; i < 4; i++)
		{
			float x = buttonsLeft + btn * 0.5f + i * (btn + gap);
			int src = 3 - i;
			placeRole(P2A + src, x, adY);
			placeRole(P2TA + src, x, turboY);
		}
		float comboW = 3.f * btn + 2.f * gap;
		float comboLeft = buttonsLeft + (faceW - comboW) * 0.5f;
		placeRole(P2ABC, comboLeft + btn * 0.5f, comboY);
		placeRole(P2CD, comboLeft + btn * 0.5f + btn + gap, comboY);
		placeRole(P2AB, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY);
		placeRole(P2TABC, comboLeft + btn * 0.5f, comboY - btn - gap);
		placeRole(P2TCD, comboLeft + btn * 0.5f + btn + gap, comboY - btn - gap);
		placeRole(P2TAB, comboLeft + btn * 0.5f + 2.f * (btn + gap), comboY - btn - gap);
		float coin = std::min(9.f, btn);
		float coinY = wide ? dpadY - dpad * 0.5f - gap - coin * 0.5f
			: std::min(comboY - btn - gap, dpadY - dpad * 0.5f) - gap - coin * 0.5f;
		placeRole(P2Start, p2RightEdge - coin * 0.5f, coinY);
		placeRole(P2Coin, p2RightEdge - coin * 1.5f - gap, coinY);
	}
	if(!wide)
	{
		float coin = std::min(9.f, std::max(b1, b2));
		float highest = bottom - (b1 + gap) * 3.f - gap - coin;
		v.setVideoBottomInsetMM(std::max(32.f, H - (highest - margin)));
	}
	else
	{
		v.setVideoBottomInsetMM(0);
	}
}

void addFaceSet(VController &v, int player, bool both, int baseRole)
{
	uint8_t set = both ? 2 : uint8_t(player);
	auto add = [&](KeyInfo key, int role)
	{
		v.addDuoButton(key, 1000, set, uint8_t(role));
	};
	v.addDuoDPad(dpadKeys(player, both), 2000, set, uint8_t(baseRole + P1Dpad));
	add(faceKey(NeoKey::A, player, both, false), baseRole + P1A);
	add(faceKey(NeoKey::B, player, both, false), baseRole + P1B);
	add(faceKey(NeoKey::C, player, both, false), baseRole + P1C);
	add(faceKey(NeoKey::D, player, both, false), baseRole + P1D);
	add(faceKey(NeoKey::A, player, both, true), baseRole + P1TA);
	add(faceKey(NeoKey::B, player, both, true), baseRole + P1TB);
	add(faceKey(NeoKey::C, player, both, true), baseRole + P1TC);
	add(faceKey(NeoKey::D, player, both, true), baseRole + P1TD);
	add(combo2(NeoKey::A, NeoKey::B, player, both, false), baseRole + P1AB);
	add(combo2(NeoKey::C, NeoKey::D, player, both, false), baseRole + P1CD);
	add(combo3(NeoKey::A, NeoKey::B, NeoKey::C, player, both, false), baseRole + P1ABC);
	add(combo2(NeoKey::A, NeoKey::B, player, both, true), baseRole + P1TAB);
	add(combo2(NeoKey::C, NeoKey::D, player, both, true), baseRole + P1TCD);
	add(combo3(NeoKey::A, NeoKey::B, NeoKey::C, player, both, true), baseRole + P1TABC);
	v.addDuoButton(faceKey(NeoKey::Select, player, false, false), 900, uint8_t(player), uint8_t(baseRole == P2Base ? P2Coin : P1Coin));
	v.addDuoButton(faceKey(NeoKey::Start, player, false, false), 900, uint8_t(player), uint8_t(baseRole == P2Base ? P2Start : P1Start));
}

}

bool AppMeta::buildDuoGamepad(VController &v)
{
	const bool joint = v.duoLayout() == DuoLayout::Joint;
	if(joint)
	{
		addFaceSet(v, 0, true, 0);
		// Per-player coin/start in the corners. addFaceSet already added P1's.
		v.addDuoButton(faceKey(NeoKey::Select, 1, false, false), 900, 1, P2Coin);
		v.addDuoButton(faceKey(NeoKey::Start, 1, false, false), 900, 1, P2Start);
	}
	else
	{
		addFaceSet(v, 0, false, 0);
		addFaceSet(v, 1, false, P2Base);
		if(v.showSharedFaceRow())
		{
			v.addDuoButton(faceKey(NeoKey::A, 0, true, false), 1000, 2, BothA);
			v.addDuoButton(faceKey(NeoKey::B, 0, true, false), 1000, 2, BothB);
			v.addDuoButton(faceKey(NeoKey::C, 0, true, false), 1000, 2, BothC);
			v.addDuoButton(faceKey(NeoKey::D, 0, true, false), 1000, 2, BothD);
		}
	}
	if(v.hasWindow())
		layoutDuoGamepad(v);
	return true;
}

void AppMeta::layoutDuoGamepad(VController &v)
{
	if(!v.hasWindow())
		return;
	float w = std::max(1.f, canvasWidthMM(v.window()));
	float h = std::max(1.f, canvasHeightMM(v.window()));
	float longMM = std::max(w, h);
	float shortMM = std::min(w, h);
	// Layout 0 is the wide (wider than 16:10) arrangement. Layout 1 is the
	// narrow arrangement, including portrait, where the game image leaves a strip.
	layoutCanvas(v, 0, longMM, shortMM, true);
	float narrowW = shortMM;
	float narrowH = longMM;
	if(longMM / shortMM <= 1.6f)
	{
		narrowW = longMM;
		narrowH = shortMM;
	}
	layoutCanvas(v, 1, narrowW, narrowH, false);
}

}
