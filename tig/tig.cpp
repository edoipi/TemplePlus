
#include "stdafx.h"
#include "tig.h"

void TigRect::FitInto(const TigRect& boundingRect) {

	/*
	Calculates the rectangle within the back buffer that the scene
	will be drawn in. This accounts for "fit to width/height" scenarios
	where the back buffer has a different aspect ratio.
	*/
	float w = static_cast<float>(boundingRect.width);
	float h = static_cast<float>(boundingRect.height);
	float wFactor = (float)w / width;
	float hFactor = (float)h / height;
	float scale = min(wFactor, hFactor);
	width = (int)round(scale * width);
	height = (int)round(scale  * height);

	// Center in bounding Rect
	x = boundingRect.x + (boundingRect.width - width) / 2;
	y = boundingRect.y + (boundingRect.height - height) / 2;
}

RECT TigRect::ToRect() {
	return {x, y, x + width, y + height};
}