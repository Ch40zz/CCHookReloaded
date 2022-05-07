#pragma once

namespace ui
{
	void AdjustFrom640(float *x, float *y, float *w, float *h);
	bool WorldToScreen(const vec3_t worldCoord, float *x, float *y);
	void DrawLine2D(float x0, float y0, float x1, float y1, float f, const vec4_t color);
	void DrawLine3D(const vec3_t from, const vec3_t to, float f, const vec4_t col);
	void DrawBox2D(float x, float y, float w, float h, const vec4_t color, qhandle_t shader);
	void DrawBox3D(const vec3_t mins, const vec3_t maxs, float f, const vec4_t color);
	void DrawBoxBordered2D(float x, float y, float w, float h, const vec4_t fillColor, const vec4_t borderColor, qhandle_t shader);
	void SizeText(float scalex, float scaley, const char *text, int limit, const fontInfo_t *font, float *width, float *height);
	void DrawText(float x, float y, float scalex, float scaley, const vec4_t color, const char *text, float adjust, int limit, int style, int align, const fontInfo_t *font);
	void DrawBoxedText(float x, float y, float scalex, float scaley, const vec4_t textColor, const char *text, float adjust, int limit, int style, int align, const fontInfo_t *font, const vec4_t bgColor, const vec4_t boColor);
	void DrawIcon(float x, float y, float w, float h, const vec4_t color, qhandle_t shader);

	void DrawMenu();
}
