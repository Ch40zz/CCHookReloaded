#include "pch.h"
#include "globals.h"
#include "ui.h"

#include "config.h"

namespace ui
{
	static int16_t btnDelete = 0, btnMouseL = 0;

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
		DoSyscall(CG_R_DRAWSTRETCHPIC, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, shader);
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

	void DrawButton(float x, float y, float w, float h, const char *text, std::function<void()> callback, int *currentTab = nullptr, int myTab = 0)
	{
		const bool IsHovering = cgDC_cursorx >= x && cgDC_cursorx < x + w &&
								cgDC_cursory >= y && cgDC_cursory < y + h;

		const bool DoDrawHovering = IsHovering || (currentTab && *currentTab == myTab);

		const vec_t* bgColor = DoDrawHovering
			? (btnMouseL < 0 && IsHovering)
				? colorMenuBgPr
				: colorMenuBgHl
			: colorMenuBg;

		const vec_t* boColor = DoDrawHovering 
			? (btnMouseL < 0 && IsHovering)
				? colorMenuBoPr
				: colorMenuBoHl
			: colorMenuBo;

		const int flags = DoDrawHovering 
			? (btnMouseL < 0 && IsHovering)
				? ITEM_TEXTSTYLE_SHADOWEDMORE
				: ITEM_TEXTSTYLE_SHADOWED
			: 0;

		DrawBoxBordered2D(x, y, w, h, bgColor, boColor, media.whiteShader);
		DrawText(x + w/2.0f, y + h/2.0f, 0.14f, 0.14f, colorWhite, 
			text, 0.0f, 0, flags, ITEM_ALIGN_CENTER2, &media.limboFont2);

		if (IsHovering && (btnMouseL & 1))
		{
			if (currentTab)
				*currentTab = myTab;

			if (callback)
				callback();
		}
	}
	void DrawCheckbox(float x, float y, const char *text, bool *state)
	{
		float textW, textH;
		SizeText(0.14f, 0.14f, text, 0, &media.limboFont2, &textW, &textH);

		const int w = 9, h = 9;
		const bool IsHovering = cgDC_cursorx >= x && cgDC_cursorx < (x + w + 2.0f + textW) &&
								cgDC_cursory >= y && cgDC_cursory < y + h;
		const vec_t *color = IsHovering ? colorCheckboxHl : colorCheckbox;

		DrawBox2D(x, y, w, h, color, *state ? media.checkboxChecked : media.checkboxUnchecked);
		DrawText(x + w + 2.0f, y + h/2 - textH/2 + 0.5, 0.14f, 0.14f, color, text, 0.0f, 0, 0, ITEM_ALIGN_LEFT, &media.limboFont2);

		if (IsHovering && (btnMouseL & 1))
			*state ^= 1;
	}
	void DrawMenu()
	{
		const int menuSizeX = 245;
		const int menuSizeY = 150;
		const int menuX = 320 - menuSizeX/2;
		const int menuY = 240 - menuSizeY/2;

		static int currentTab = 0;

		// DoSyscall(CG_KEY_ISDOWN, K_MOUSE1)?
		btnDelete = GetAsyncKeyState(VK_DELETE);
		btnMouseL = GetAsyncKeyState(VK_LBUTTON);

		DrawBoxedText(10.0f, 10.0f, 0.16f, 0.16f, colorWhite, XorString("^1CC^7Hook^1:^9Reloaded"), 
			0.0f, 0, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_LEFT, &media.limboFont1, colorMenuBg, colorMenuBo);

		if ((btnDelete & 1) && !(DoSyscall(CG_KEY_GETCATCHER) & (KEYCATCH_CONSOLE | KEYCATCH_UI)))
		{
			showMenu = !showMenu;

			// Disable user input block and mouse capturing
			if (!showMenu)
				DoSyscall(CG_KEY_SETCATCHER, DoSyscall(CG_KEY_GETCATCHER) & ~(KEYCATCH_MESSAGE | KEYCATCH_CGAME));
		}

		if (!showMenu)
			return;

		// Block user input for CGame and capture mouse move events
		DoSyscall(CG_KEY_SETCATCHER, DoSyscall(CG_KEY_GETCATCHER) | KEYCATCH_CGAME | KEYCATCH_MESSAGE);


		DrawBoxBordered2D(menuX, menuY, menuSizeX, menuSizeY, colorMenuBg, colorMenuBo, media.whiteShader);
		DrawBox2D(menuX, menuY, menuSizeX, 12.0f, colorMenuBg, media.whiteShader);
		DrawText(menuX + menuSizeX/2.0f, menuY + 3.5f, 0.16f, 0.16f, colorWhite, 
			XorString("^1CC^7Hook^1:^9Reloaded"), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, ITEM_ALIGN_CENTER, &media.limboFont1);
		DrawBox2D(menuX, menuY + 12, menuSizeX, 0.5f, colorMenuBo, media.whiteShader);

		DrawButton(menuX + 5,   menuY + 17, 55, 10, XorString("Aimbot"), nullptr, &currentTab, 0);
		DrawButton(menuX + 65,  menuY + 17, 55, 10, XorString("Visuals"), nullptr, &currentTab, 1);
		DrawButton(menuX + 125, menuY + 17, 55, 10, XorString("ESP"), nullptr, &currentTab, 2);
		DrawButton(menuX + 185, menuY + 17, 55, 10, XorString("Misc"), nullptr, &currentTab, 3);

		DrawBoxBordered2D(menuX + 5, menuY + 31, menuSizeX - 10, menuSizeY - 35, colorMenuBg, colorMenuBo, media.whiteShader);

		switch (currentTab)
		{
		case 0: // Aimbot
			DrawCheckbox(menuX + 10, menuY + 35, XorString("Aimbot Enabled"), &cfg.aimbotEnabled);
			DrawCheckbox(menuX + 10, menuY + 45, XorString("Sticky Aim"), &cfg.aimbotStickyAim);
			DrawCheckbox(menuX + 10, menuY + 55, XorString("Sticky Auto-Reset"), &cfg.aimbotStickyAutoReset);
			DrawCheckbox(menuX + 10, menuY + 65, XorString("Lock Viewangles"), &cfg.aimbotLockViewangles);
			DrawCheckbox(menuX + 10, menuY + 75, XorString("Autoshoot"), &cfg.aimbotAutoshoot);
			DrawCheckbox(menuX + 10, menuY + 85, XorString("Anti-Lagcompensation"), &cfg.aimbotAntiLagCompensation);
			DrawCheckbox(menuX + 10, menuY + 95, XorString("Human Aim"), &cfg.aimbotHumanAim);
			break;
		case 1: // Visuals
			DrawCheckbox(menuX + 10, menuY + 35, XorString("Scoped Walk"), &cfg.scopedWalk);
			DrawCheckbox(menuX + 10, menuY + 45, XorString("No Scope-FoV"), &cfg.noScopeFov);
			DrawCheckbox(menuX + 10, menuY + 55, XorString("No Scope-Blackout"), &cfg.noScopeBlackout);
			DrawCheckbox(menuX + 10, menuY + 65, XorString("Bullet Tracers"), &cfg.bulletTracers);
			DrawCheckbox(menuX + 10, menuY + 75, XorString("Grenade Trajectory"), &cfg.grenadeTrajectory);
			DrawCheckbox(menuX + 10, menuY + 85, XorString("No Damage Feedback"), &cfg.noDamageFeedback);
			DrawCheckbox(menuX + 10, menuY + 95, XorString("No Camera Shake"), &cfg.noCamExplosionShake);
			DrawCheckbox(menuX + 10, menuY + 105, XorString("No Smoke"), &cfg.noSmoke);
			DrawCheckbox(menuX + 10, menuY + 115, XorString("No Foliage"), &cfg.noFoliage);
			DrawCheckbox(menuX + 10, menuY + 125, XorString("No Weather"), &cfg.noWeather);

			DrawCheckbox(menuX + 120, menuY + 35, XorString("Pickup Chams"), &cfg.pickupChams);
			DrawCheckbox(menuX + 120, menuY + 45, XorString("Missile Chams"), &cfg.missileChams);
			DrawCheckbox(menuX + 120, menuY + 55, XorString("Player Chams"), &cfg.playerChams);
			DrawCheckbox(menuX + 120, menuY + 65, XorString("Player Outline"), &cfg.playerOutlineChams);
			DrawCheckbox(menuX + 120, menuY + 75, XorString("Player Corpse"), &cfg.playerCorpseChams);
			break;
		case 2: // ESP
			DrawCheckbox(menuX + 10, menuY + 35, XorString("Head BBox"), &cfg.headBbox);
			DrawCheckbox(menuX + 10, menuY + 45, XorString("Body BBox"), &cfg.bodyBbox);
			DrawCheckbox(menuX + 10, menuY + 55, XorString("Bone ESP"), &cfg.boneEsp);
			DrawCheckbox(menuX + 10, menuY + 65, XorString("Name ESP"), &cfg.nameEsp);
			DrawCheckbox(menuX + 10, menuY + 75, XorString("Missile ESP"), &cfg.missileEsp);
			DrawCheckbox(menuX + 10, menuY + 85, XorString("Missile Radius"), &cfg.missileRadius);
			DrawCheckbox(menuX + 10, menuY + 95, XorString("Pickup ESP"), &cfg.pickupEsp);
			break;
		case 3: // Misc
			DrawCheckbox(menuX + 10, menuY + 35, XorString("Spectator Warning"), &cfg.spectatorWarning);
			DrawCheckbox(menuX + 10, menuY + 45, XorString("Enemy Spawntimer"), &cfg.enemySpawnTimer);
			DrawCheckbox(menuX + 10, menuY + 55, XorString("Custom Damage Sounds"), &cfg.customDmgSounds);
			DrawCheckbox(menuX + 10, menuY + 65, XorString("Quick Unban-Reconnect"), &cfg.quickUnbanReconnect);
			DrawCheckbox(menuX + 10, menuY + 75, XorString("Clean Screenshots"), &cfg.cleanScreenshots);
			break;
		}

		ui::DrawBox2D(cgDC_cursorx, cgDC_cursory, 32, 32, colorWhite, media.cursorIcon);
	}
}
