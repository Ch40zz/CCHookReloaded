#include "pch.h"
#include "globals.h"
#include "ui.h"

namespace ui
{
	void AdjustFrom640(float *x, float *y, float *w, float *h)
	{
		*x *= cg_glconfig.vidWidth / 640.0f;
		*y *= cg_glconfig.vidHeight / 480.0f;

		*w *= cg_glconfig.vidWidth / 640.0f;
		*h *= cg_glconfig.vidHeight / 480.0f;
	}
	bool WorldToScreen(const vec3_t worldCoord, float *x, float *y)
	{
		vec3_t trans;
		VectorSubtract(worldCoord, cg_refdef.vieworg, trans);

		// z = how far is the object in our forward direction, negative = behind/mirrored
		float z = DotProduct(trans, cg_refdef.viewaxis[0]);
		if (z <= 0.001)
			return false;

		float xc = 640.0 / 2.0;
		float yc = 480.0 / 2.0;

		float px = tan(cg_refdef.fov_x * (M_PI / 360));
		float py = tan(cg_refdef.fov_y * (M_PI / 360));

		*x = xc - DotProduct(trans, cg_refdef.viewaxis[1])*xc/(z*px);
		*y = yc - DotProduct(trans, cg_refdef.viewaxis[2])*yc/(z*py);

		return true;
	}
	void DrawLine2D(float x0, float y0, float x1, float y1, float f, const vec4_t color)
	{
		ui::AdjustFrom640(&x0, &y0, &x1, &y1);

		vec2_t org;
		org[0] = (x0+x1)/2.0f;
		org[1] = (y0+y1)/2.0f;

		float w = sqrt((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));
		float angle = atan2((x1-x0), (y1-y0));

		polyVert_t verts[4];
		SetupRotatedThing(verts, org, w, f, angle);

		// Set color for all vertices
		for (size_t i = 0; i < std::size(verts); i++)
		{
			verts[i].modulate[0] = color[0] * 255;
			verts[i].modulate[1] = color[1] * 255;
			verts[i].modulate[2] = color[2] * 255;
			verts[i].modulate[3] = 255;
		}
	
		DoSyscall(CG_R_DRAW2DPOLYS, verts, 4, media.whiteShader);
	}
	void DrawLine3D(const vec3_t from, const vec3_t to, float f, const vec4_t col)
	{
		float fromX, fromY, 
			toX, toY;

		if (WorldToScreen(from, &fromX, &fromY) && WorldToScreen(to, &toX, &toY))
			DrawLine2D(fromX, fromY, toX, toY, f, col);
	}
	void DrawBox2D(float x, float y, float w, float h, const vec4_t color, qhandle_t shader)
	{
		AdjustFrom640(&x, &y, &w, &h);

		DoSyscall(CG_R_SETCOLOR, color);
		DoSyscall(CG_R_DRAWSTRETCHPIC, x, y, w, h, 0.0f, 0.0f, 0.0f, 1.0f, shader);
		DoSyscall(CG_R_SETCOLOR, nullptr);
	}
	void DrawBox3D(const vec3_t mins, const vec3_t maxs, float f, const vec4_t color)
	{
		vec3_t boundingBox[] =
		{
			{ maxs[0], maxs[1], maxs[2] },
			{ maxs[0], maxs[1], mins[2] },
			{ mins[0], maxs[1], mins[2] },
			{ mins[0], maxs[1], maxs[2] },
			{ maxs[0], mins[1], maxs[2] },
			{ maxs[0], mins[1], mins[2] },
			{ mins[0], mins[1], mins[2] },
			{ mins[0], mins[1], maxs[2] }
		};

		//Upper rect
		DrawLine3D(boundingBox[0], boundingBox[1], f, color);
		DrawLine3D(boundingBox[0], boundingBox[3], f, color);
		DrawLine3D(boundingBox[2], boundingBox[1], f, color);
		DrawLine3D(boundingBox[2], boundingBox[3], f, color);

		//Upper to lower rect
		DrawLine3D(boundingBox[0], boundingBox[4], f, color);
		DrawLine3D(boundingBox[1], boundingBox[5], f, color);
		DrawLine3D(boundingBox[2], boundingBox[6], f, color);
		DrawLine3D(boundingBox[3], boundingBox[7], f, color);

		//Lower rect
		DrawLine3D(boundingBox[4], boundingBox[5], f, color);
		DrawLine3D(boundingBox[4], boundingBox[7], f, color);
		DrawLine3D(boundingBox[6], boundingBox[5], f, color);
		DrawLine3D(boundingBox[6], boundingBox[7], f, color);
	}
	void DrawBoxBordered2D(float x, float y, float w, float h, const vec4_t fillColor, const vec4_t borderColor, qhandle_t shader)
	{
		const float borderSize = 1.0f;

		AdjustFrom640(&x, &y, &w, &h);

		DoSyscall(CG_R_SETCOLOR, fillColor);
		DoSyscall(CG_R_DRAWSTRETCHPIC, x, y, w, h, 0.0f, 0.0f, 0.0f, 1.0f, shader);
	
		DoSyscall(CG_R_SETCOLOR, borderColor);
		DoSyscall(CG_R_DRAWSTRETCHPIC, x-borderSize, y-borderSize, w+borderSize*2, borderSize, 0.0f, 0.0f, 0.0f, 1.0f, shader);
		DoSyscall(CG_R_DRAWSTRETCHPIC, x-borderSize, y+h, w+borderSize*2, borderSize, 0.0f, 0.0f, 0.0f, 1.0f, shader);
		DoSyscall(CG_R_DRAWSTRETCHPIC, x-borderSize, y, borderSize, h, 0.0f, 0.0f, 0.0f, 1.0f, shader);
		DoSyscall(CG_R_DRAWSTRETCHPIC, x+w, y, borderSize, h, 0.0f, 0.0f, 0.0f, 1.0f, shader);

		DoSyscall(CG_R_SETCOLOR, nullptr);
	}
	void SizeText(float scalex, float scaley, const char *text, int limit, const fontInfo_t *font, float *width, float *height)
	{
		int len = strlen(text);
		if (limit > 0 && len > limit)
			len = limit;
	
		const uint8_t *s = (uint8_t*)text;

		float w = 0.0f;
		float h = 0.0f;

		for (int count = 0; *s && count < len; )
		{
			if (Q_IsColorString(s))
			{
				s += 2;
				continue;
			}

			const glyphInfo_t &glyph = font->glyphs[*s];
			w += glyph.xSkip;

			if (glyph.height > h)
				h = glyph.height;

			count++;
			s++;
		}

		*width = w * font->glyphScale * scalex;
		*height = h * font->glyphScale * scaley;
	}
	void DrawText(float x, float y, float scalex, float scaley, const vec4_t color, const char *text, float adjust, int limit, int style, int align, const fontInfo_t *font)
	{
		float w, h;
		SizeText(scalex, scaley, text, limit, font, &w, &h);

		if (align == ITEM_ALIGN_CENTER2)
			y += h / 2.0f;
		else
			y += h;

		if (align == ITEM_ALIGN_CENTER || align == ITEM_ALIGN_CENTER2)
			x -= w / 2.0f;
		else if (align == ITEM_ALIGN_RIGHT)
			x -= w;

		scalex *= font->glyphScale;
		scaley *= font->glyphScale;

		const uint8_t *s = (uint8_t*)text;
		int len = strlen(text);

		if (limit > 0 && len > limit)
			len = limit;

		vec4_t newColor;
		Vector4Copy(color, newColor);
		DoSyscall(CG_R_SETCOLOR, color);

		for (int count = 0; *s && count < len; )
		{
			const glyphInfo_t &glyph = font->glyphs[*s];

			if (Q_IsColorString(s))
			{
				const uint8_t newColorChar = *(s + 1);

				// Do not copy alpha value ever again, only initialized once
				if(newColorChar == COLOR_NULL)
					VectorCopy(color, newColor);
				else
					VectorCopy(g_color_table[ColorIndex(newColorChar)], newColor);

				DoSyscall(CG_R_SETCOLOR, newColor);

				s += 2;
				continue;
			}

			float x_adj = x + glyph.pitch*scalex;
			float y_adj = y - glyph.top*scaley;
			float w_adj = glyph.imageWidth*scalex;
			float h_adj = glyph.imageHeight*scaley;

			AdjustFrom640(&x_adj, &y_adj, &w_adj, &h_adj);

			// Draw the drop shadow first
			if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
			{
				vec4_t outlineCol = {0, 0, 0, newColor[3]};
				float ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1.0f : 2.0f;
			
				DoSyscall(CG_R_SETCOLOR, outlineCol);
				DoSyscall(CG_R_DRAWSTRETCHPIC, x_adj + ofs, y_adj + ofs, w_adj, h_adj, glyph.s, glyph.t, glyph.s2, glyph.t2, glyph.glyph);
				DoSyscall(CG_R_SETCOLOR, newColor);
			}

			DoSyscall(CG_R_DRAWSTRETCHPIC, x_adj, y_adj, w_adj, h_adj, glyph.s, glyph.t, glyph.s2, glyph.t2, glyph.glyph);

			x += (glyph.xSkip * scalex) + adjust;
			count++;
			s++;
		}

		DoSyscall(CG_R_SETCOLOR, nullptr);
	}
	void DrawBoxedText(float x, float y, float scalex, float scaley, const vec4_t textColor, const char *text, float adjust, int limit, int style, int align, const fontInfo_t *font, const vec4_t bgColor, const vec4_t boColor)
	{
		const float boxPadding = 7.0f;

		float w, h;
		SizeText(scalex, scaley, text, limit, font, &w, &h);

		float _x = x, _y = y;

		if (align == ITEM_ALIGN_CENTER2)
			_y += h / 2.0f;

		if (align == ITEM_ALIGN_CENTER || align == ITEM_ALIGN_CENTER2)
			_x -= w / 2.0f;
		else if (align == ITEM_ALIGN_RIGHT)
			_x -= w;

		DrawBoxBordered2D(_x, _y, w + boxPadding, h + boxPadding, bgColor, boColor, media.whiteShader);

		DrawText(x + boxPadding/2.0f, y + boxPadding/2.0f, scalex, scaley, textColor, text, adjust, limit, style, align, font);
	}
	void DrawIcon(float x, float y, float w, float h, const vec4_t color, qhandle_t shader)
	{
		AdjustFrom640(&x, &y, &w, &h);

		DoSyscall(CG_R_SETCOLOR, color);
		DoSyscall(CG_R_DRAWSTRETCHPIC, x - w/2.0f, y - h/2.0f, w, h, 0.0f, 0.0f, 1.0f, 1.0f, shader);
		DoSyscall(CG_R_SETCOLOR, nullptr);
	}
}
