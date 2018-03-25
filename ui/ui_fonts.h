#pragma once

#include "ui/menudef.h"

struct Font {
	qhandle_t	handle;
	float		scale;
	bool		customFont;

	Font(
		qhandle_t fontHandle,
		float fontScale,
		bool isCustomFont = false
	);

	float Width(
		const char *text
	) const;

	float Height(
		const char *text = nullptr // doesn't not actually read `text`
	) const;

	void Paint(
		float x,
		float y,
		const char *text,
		const vector4 *colour = &colorTable[CT_WHITE],
		uiTextStyle_e style = Normal,
		int limit = 0,
		float adjust = 0.0f
	) const;
};
