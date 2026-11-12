/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include "PlaceVideoView.hh"
#include <emuframework/EmuApp.hh>

namespace EmuEx
{

PlaceVideoView::PlaceVideoView(ViewAttachParams attach, EmuVideoLayer &layer, VController &vController):
	View(attach),
	layer{layer},
	vController{vController},
	exitText{attach.rendererTask, "Exit", &defaultFace()},
	resetText{attach.rendererTask, "Reset", &defaultFace()},
	smallerText{attach.rendererTask, "Smaller", &defaultFace()},
	largerText{attach.rendererTask, "Larger", &defaultFace()},
	quads{attach.rendererTask, {.size = 6}}
{
	app().applyOSNavStyle(appContext(), true);
	layer.setBrightnessScale(1.f);
}

PlaceVideoView::~PlaceVideoView()
{
	app().applyOSNavStyle(appContext(), false);
	layer.setBrightnessScale(menuVideoBrightnessScale);
}

void PlaceVideoView::place()
{
	dragState = {};
	exitText.compile();
	resetText.compile();
	smallerText.compile();
	largerText.compile();
	WRect btnBounds{{0, 0}, {viewRect().xSize(), exitText.nominalHeight() * 2}};
	btnBounds.setPos(viewRect().pos(LB2DO), LB2DO);
	int quarter = viewRect().xSize() / 4;
	exitBounds = btnBounds;
	exitBounds.x2 = quarter;
	resetBounds = btnBounds;
	resetBounds.x = quarter;
	resetBounds.x2 = quarter * 2;
	smallerBounds = btnBounds;
	smallerBounds.x = quarter * 2;
	smallerBounds.x2 = quarter * 3;
	largerBounds = btnBounds;
	largerBounds.x = quarter * 3;
	const int lineSize = 1;
	using Quad = decltype(quads)::Type;
	auto map = quads.map();
	Quad{{.bounds = WRect{{viewRect().x, viewRect().yCenter()},
		{viewRect().x2, viewRect().yCenter() + lineSize}}.as<int16_t>()}}.write(map, 0);
	Quad{{.bounds = WRect{{viewRect().xCenter(), viewRect().y},
		{viewRect().xCenter() + lineSize, viewRect().y2}}.as<int16_t>()}}.write(map, 1);
	Quad{{.bounds = exitBounds.as<int16_t>()}}.write(map, 2);
	Quad{{.bounds = resetBounds.as<int16_t>()}}.write(map, 3);
	Quad{{.bounds = smallerBounds.as<int16_t>()}}.write(map, 4);
	Quad{{.bounds = largerBounds.as<int16_t>()}}.write(map, 5);
}

bool PlaceVideoView::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	return e.visit(overloaded
	{
		[&](const Input::KeyEvent &e)
		{
			int x = landscapePlacement() ? layer.landscapeOffset : layer.portraitOffsetX;
			int y = landscapePlacement() ? layer.landscapeOffsetY : layer.portraitOffset;
			if(e.pushed(Input::DefaultKey::LEFT))
			{
				updateVideo(x - 1, y);
				return true;
			}
			else if(e.pushed(Input::DefaultKey::RIGHT))
			{
				updateVideo(x + 1, y);
				return true;
			}
			else if(e.pushed(Input::DefaultKey::UP))
			{
				updateVideo(x, y - 1);
				return true;
			}
			else if(e.pushed(Input::DefaultKey::DOWN))
			{
				updateVideo(x, y + 1);
				return true;
			}
			else if(e.pushed(Input::DefaultKey::CONFIRM))
			{
				resetPlacement();
				return true;
			}
			else if(e.pushed(Input::DefaultKey::CANCEL))
			{
				dismiss();
				return true;
			}
			return false;
		},
		[&](const Input::MotionEvent &e)
		{
			auto id = e.pointerId();
			switch(e.state())
			{
				case Input::Action::PUSHED:
				{
					bool onButton = exitBounds.overlaps(e.pos()) || resetBounds.overlaps(e.pos())
						|| smallerBounds.overlaps(e.pos()) || largerBounds.overlaps(e.pos());
					if(!dragState.isTracking() && !onButton)
					{
						dragState.start(id, e.pos());
						startOffsetX = landscapePlacement() ? layer.landscapeOffset : layer.portraitOffsetX;
						startOffsetY = landscapePlacement() ? layer.landscapeOffsetY : layer.portraitOffset;
					}
					break;
				}
				case Input::Action::MOVED:
				{
					if(dragState.isTracking(id))
					{
						dragState.update(e.pos(), 0);
						updateVideo(startOffsetX + dragState.downPosDiff().x, startOffsetY + dragState.downPosDiff().y);
					}
					break;
				}
				case Input::Action::RELEASED:
				case Input::Action::CANCELED:
				{
					if(dragState.isTracking(id))
					{
						dragState = {};
					}
					else if(e.released())
					{
						if(exitBounds.overlaps(e.pos()))
						{
							dismiss();
						}
						else if(resetBounds.overlaps(e.pos()))
						{
							resetPlacement();
						}
						else if(smallerBounds.overlaps(e.pos()))
						{
							int scale = landscapePlacement() ? layer.landscapePlaceScale : layer.portraitPlaceScale;
							updateScale(scale - 10);
						}
						else if(largerBounds.overlaps(e.pos()))
						{
							int scale = landscapePlacement() ? layer.landscapePlaceScale : layer.portraitPlaceScale;
							updateScale(scale + 10);
						}
					}
					break;
				}
				default:
					break;
			}
			return true;
		}
	});
}

void PlaceVideoView::draw(Gfx::RendererCommands&__restrict__ cmds, ViewDrawParams) const
{
	using namespace Gfx;
	vController.draw(cmds, true);
	cmds.setColor({.5, .5, .5});
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	cmds.setVertexArray(quads);
	cmds.drawQuads(0, 2); // centering lines
	cmds.setColor({.2, .2, .2, .5});
	cmds.drawQuads(2, 4); // button bg
	basicEffect.enableAlphaTexture(cmds);
	cmds.setColor(ColorName::WHITE);
	exitText.draw(cmds, exitBounds.pos(C2DO), C2DO);
	resetText.draw(cmds, resetBounds.pos(C2DO), C2DO);
	smallerText.draw(cmds, smallerBounds.pos(C2DO), C2DO);
	largerText.draw(cmds, largerBounds.pos(C2DO), C2DO);
}

bool PlaceVideoView::landscapePlacement() const
{
	return viewRect().isLandscape();
}

void PlaceVideoView::updateVideo(int x, int y)
{
	int limitX = std::min(16000, std::max(1, viewRect().xSize()));
	int limitY = std::min(16000, std::max(1, viewRect().ySize()));
	x = std::clamp(x, -limitX, limitX);
	y = std::clamp(y, -limitY, limitY);
	if(landscapePlacement())
	{
		layer.landscapeOffset = int16_t(x);
		layer.landscapeOffsetY = int16_t(y);
	}
	else
	{
		layer.portraitOffsetX = int16_t(x);
		layer.portraitOffset = int16_t(y);
	}
	app().viewController().placeEmuViews();
	app().viewController().postDrawToEmuWindows();
}

void PlaceVideoView::updateScale(int scale)
{
	scale = std::clamp(scale, 25, 300);
	if(landscapePlacement())
		layer.landscapePlaceScale = int16_t(scale);
	else
		layer.portraitPlaceScale = int16_t(scale);
	app().viewController().placeEmuViews();
	app().viewController().postDrawToEmuWindows();
}

void PlaceVideoView::resetPlacement()
{
	if(landscapePlacement())
	{
		layer.landscapeOffset = 0;
		layer.landscapeOffsetY = 0;
		layer.landscapePlaceScale = 100;
	}
	else
	{
		layer.portraitOffsetX = 0;
		layer.portraitOffset = 0;
		layer.portraitPlaceScale = 100;
	}
	app().viewController().placeEmuViews();
	app().viewController().postDrawToEmuWindows();
}

void PlaceVideoView::onShow()
{
	vController.applyButtonAlpha(.50);
}

}
