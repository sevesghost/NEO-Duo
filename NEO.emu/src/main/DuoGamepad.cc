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
	BothA = 64, BothB, BothC, BothD,
	BothSelect, BothStart
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

struct Pack
{
	float b1, b2, shared, gap, margin;
};

Pack packSizes(const VController &v, float canvasW, bool joint)
{
	// Buttons never go below 9 mm. When the row is wider than the screen,
	// close the gaps and then the edge margin.
	float b1 = std::max(9.f, v.playerFaceSizeMM100x(0) / 100.f);
	float b2 = std::max(9.f, v.playerFaceSizeMM100x(1) / 100.f);
	float shared = std::max(9.f, v.sharedFaceSizeMM100x() / 100.f);
	float gap = 2.f;
	float margin = 5.f;
	auto needed = [&](float m, float g)
	{
		if(joint)
			return m + b1 * 2.f + g + 4.f * b1 + 3.f * g + m;
		float left = b1 * 2.f + g + 4.f * b1 + 3.f * g;
		float right = b2 * 2.f + g + 4.f * b2 + 3.f * g;
		return m + left + g + right + m;
	};
	while(gap > 0.f && needed(margin, gap) > canvasW)
		gap = std::max(0.f, gap - 0.25f);
	while(margin > 0.f && needed(margin, gap) > canvasW)
		margin = std::max(0.f, margin - 0.5f);
	return {b1, b2, shared, gap, margin};
}

void assignSizes(VController &v, const Pack &p)
{
	for(auto &elem : v.deviceElements())
	{
		if(elem.duoRole == 255)
			continue;
		float face = elem.playerSet() == 1 ? p.b2 : elem.playerSet() == 2 ? p.shared : p.b1;
		bool dpad = elem.dPad() != nullptr;
		uint8_t role = elem.duoRole;
		bool coin = role == P1Coin || role == P1Start || role == P2Coin || role == P2Start;
		float size = coin ? 9.f : dpad ? face * 2.f : face;
		if(size < 9.f)
			size = 9.f;
		elem.sizeMM100x = int16_t(std::lround(size * 100.f));
	}
}

struct Rows
{
	float ad, turbo, combo, tcombo;
};

// adFromBottom is the A–D center. At the default 10 mm face the turbo row is
// 10.8 mm above that (9 mm and 19.8 mm). The turbo row always stays inside
// the bottom 40% of the screen. When that band cannot hold every row, the
// plain combo row sits above the ceiling instead of being crushed onto turbo.
Rows makeRows(float btn, float gap, float ceiling, float adFromBottom)
{
	float half = btn * 0.5f;
	if(adFromBottom < half)
		adFromBottom = half;
	float step = btn + std::max(gap, 2.f);
	// 10.8 mm is the reference rise. A larger face needs a full step or the
	// two rows overlap.
	float turboRise = btn <= 10.f ? 10.8f : step;
	float turboLimit = std::max(half, ceiling - half);
	float classicTurbo = adFromBottom + turboRise;
	float classicCombo = classicTurbo + step;
	float classicTcombo = classicCombo + step;
	if(classicTcombo + half <= ceiling)
		return {adFromBottom, classicTurbo, classicCombo, classicTcombo};
	float turbo = std::min(classicTurbo, turboLimit);
	float ad = adFromBottom;
	if(std::abs(ad - turbo) < btn)
		ad = turbo + step;
	float above = std::max(ad, turbo);
	return {ad, turbo, above + step, above + step * 2.f};
}

void layoutCanvas(VController &v, int layout, float W, float H, bool wide)
{
	const bool joint = v.duoLayout() == DuoLayout::Joint;
	const bool shared = !joint && v.showSharedFaceRow();
	Pack p = packSizes(v, W, joint);
	assignSizes(v, p);
	float b1 = p.b1, b2 = p.b2, gap = p.gap, margin = p.margin;
	float coin = 9.f;
	float ceiling = 0.4f * H;

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

	auto placeRows = [&](int base, float buttonsLeft, float btn, const Rows &rows, bool mirror)
	{
		for(int i = 0; i < 4; i++)
		{
			float x = buttonsLeft + btn * 0.5f + i * (btn + gap);
			int src = mirror ? 3 - i : i;
			placeRole(base + P1A + src, x, H - rows.ad);
			placeRole(base + P1TA + src, x, H - rows.turbo);
		}
		float faceW = 4.f * btn + 3.f * gap;
		float comboW = 3.f * btn + 2.f * gap;
		float comboLeft = buttonsLeft + (faceW - comboW) * 0.5f;
		auto comboX = [&](int slot) { return comboLeft + btn * 0.5f + slot * (btn + gap); };
		if(!mirror)
		{
			placeRole(base + P1AB, comboX(0), H - rows.combo);
			placeRole(base + P1CD, comboX(1), H - rows.combo);
			placeRole(base + P1ABC, comboX(2), H - rows.combo);
			placeRole(base + P1TAB, comboX(0), H - rows.tcombo);
			placeRole(base + P1TCD, comboX(1), H - rows.tcombo);
			placeRole(base + P1TABC, comboX(2), H - rows.tcombo);
		}
		else
		{
			placeRole(base + P1ABC, comboX(0), H - rows.combo);
			placeRole(base + P1CD, comboX(1), H - rows.combo);
			placeRole(base + P1AB, comboX(2), H - rows.combo);
			placeRole(base + P1TABC, comboX(0), H - rows.tcombo);
			placeRole(base + P1TCD, comboX(1), H - rows.tcombo);
			placeRole(base + P1TAB, comboX(2), H - rows.tcombo);
		}
	};

	float topY = margin + coin * 0.5f;
	placeRole(P1Coin, margin + coin * 0.5f, topY);
	placeRole(P1Start, margin + coin * 1.5f + gap, topY);
	placeRole(P2Coin, W - margin - coin * 1.5f - gap, topY);
	placeRole(P2Start, W - margin - coin * 0.5f, topY);

	float clusterTop = 0.f;
	if(joint)
	{
		float dpad = b1 * 2.f;
		float dpadX = margin + dpad * 0.5f;
		float dpadFromBottom = margin + dpad * 0.5f;
		placeRole(P1Dpad, dpadX, H - dpadFromBottom);
		float pair = coin + gap;
		placeRole(BothSelect, W * 0.5f - pair * 0.5f, topY);
		placeRole(BothStart, W * 0.5f + pair * 0.5f, topY);
		// Landscape joint keeps the face buttons on the right so the left
		// hand stays on the D-pad. Centers are millimetres from the
		// bottom-right corner, matching a 10 mm face on a 151x68 mm phone.
		// dx grows to the left. The top row is staggered 6 mm from the bottom.
		float pitch = std::max(12.f, b1 + 2.f);
		float s = pitch / 12.f;
		float dyBottom = std::max(b1 * 0.5f, 9.f * s);
		float dyTop = dyBottom + (b1 <= 10.f ? 10.8f * s : std::max(10.8f * s, b1 + 2.f));
		// The top row holds the turbo buttons, so its top edge stays inside 40%.
		float dyTopLimit = std::max(dyBottom + b1, ceiling - b1 * 0.5f);
		if(dyTop > dyTopLimit)
			dyTop = dyTopLimit;
		struct Slot { uint8_t role; float dx; float dy; };
		Slot slots[]
		{
			{P1D, 12, 0}, {P1C, 24, 0}, {P1B, 36, 0}, {P1A, 48, 0},
			{P1CD, 72, 0}, {P1ABC, 84, 0}, {P1TABC, 96, 0},
			{P1TD, 18, 1}, {P1TC, 30, 1}, {P1TB, 42, 1}, {P1TA, 54, 1},
			{P1AB, 66, 1}, {P1TAB, 78, 1}, {P1TCD, 90, 1},
		};
		float leftmost = 96.f * s + b1 * 0.5f;
		bool horizontal = W >= H && (W - leftmost) >= (margin + dpad + gap);
		if(horizontal)
		{
			for(auto slot : slots)
			{
				float dx = slot.dx * s;
				float dy = slot.dy ? dyTop : dyBottom;
				placeRole(slot.role, W - dx, H - dy);
			}
			clusterTop = std::max(dpadFromBottom + dpad * 0.5f, dyTop + b1 * 0.5f);
		}
		else
		{
			// Portrait is too narrow for that row, so the same buttons stack
			// on the right of the D-pad. Turbo still stays inside 40%.
			float faceW = 4.f * b1 + 3.f * gap;
			float buttonsLeft = W - margin - faceW;
			float dpadRight = margin + dpad;
			if(buttonsLeft < dpadRight + gap)
				buttonsLeft = dpadRight + gap;
			Rows rows = makeRows(b1, gap, ceiling, std::max(9.f, b1 * 0.5f));
			placeRows(0, buttonsLeft, b1, rows, false);
			clusterTop = std::max(dpadFromBottom + dpad * 0.5f, rows.tcombo + b1 * 0.5f);
		}
	}
	else
	{
		float sharedFromBottom = margin + p.shared * 0.5f;
		float ad1 = std::max(9.f, b1 * 0.5f);
		float ad2 = std::max(9.f, b2 * 0.5f);
		if(shared)
		{
			ad1 = sharedFromBottom + p.shared * 0.5f + 12.f + b1 * 0.5f;
			ad2 = sharedFromBottom + p.shared * 0.5f + 12.f + b2 * 0.5f;
			float total = 4.f * p.shared + 3.f * gap;
			float x0 = (W - total) * 0.5f + p.shared * 0.5f;
			float y = H - sharedFromBottom;
			placeRole(BothA, x0, y);
			placeRole(BothB, x0 + (p.shared + gap), y);
			placeRole(BothC, x0 + 2.f * (p.shared + gap), y);
			placeRole(BothD, x0 + 3.f * (p.shared + gap), y);
			clusterTop = sharedFromBottom + p.shared * 0.5f;
		}
		float d1 = b1 * 2.f;
		float d2 = b2 * 2.f;
		float dpadFromBottom1 = margin + d1 * 0.5f;
		float dpadFromBottom2 = margin + d2 * 0.5f;
		float dpadX1 = margin + d1 * 0.5f;
		placeRole(P1Dpad, dpadX1, H - dpadFromBottom1);
		Rows rows1 = makeRows(b1, gap, ceiling, ad1);
		placeRows(0, dpadX1 + d1 * 0.5f + gap, b1, rows1, false);
		float dpadX2 = W - margin - d2 * 0.5f;
		float faceW2 = 4.f * b2 + 3.f * gap;
		float buttonsLeft2 = dpadX2 - d2 * 0.5f - gap - faceW2;
		placeRole(P2Dpad, dpadX2, H - dpadFromBottom2);
		Rows rows2 = makeRows(b2, gap, ceiling, ad2);
		placeRows(P2Base, buttonsLeft2, b2, rows2, true);
		clusterTop = std::max(clusterTop, std::max(dpadFromBottom1 + d1 * 0.5f, rows1.tcombo + b1 * 0.5f));
		clusterTop = std::max(clusterTop, std::max(dpadFromBottom2 + d2 * 0.5f, rows2.tcombo + b2 * 0.5f));
	}
	v.setVideoBottomInsetMM(layout, wide ? 0.f : clusterTop + 2.f);
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
		// Per-player coin/start stay in the corners. Shared Select/Start sit at top center.
		v.addDuoButton(faceKey(NeoKey::Select, 1, false, false), 900, 1, P2Coin);
		v.addDuoButton(faceKey(NeoKey::Start, 1, false, false), 900, 1, P2Start);
		v.addDuoButton(faceKey(NeoKey::Select, 0, true, false), 900, 2, BothSelect);
		v.addDuoButton(faceKey(NeoKey::Start, 0, true, false), 900, 2, BothStart);
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
	bool portraitNow = h > w;
	float landW = portraitNow ? h : w;
	float landH = portraitNow ? w : h;
	// Index 0 is landscape. Index 1 is portrait, computed from the swapped edges
	// so a rotated tablet does not reuse the landscape anchors.
	layoutCanvas(v, 0, landW, landH, (landW / landH) > 1.6f);
	layoutCanvas(v, 1, landH, landW, false);
}

void AppMeta::refreshDuoButtonSizes(VController &v)
{
	Pack p = packSizes(v, 10000.f, v.duoLayout() == DuoLayout::Joint);
	assignSizes(v, p);
}

}
