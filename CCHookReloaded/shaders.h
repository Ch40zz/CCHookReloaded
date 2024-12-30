#pragma once

#define SHADER_COVER_SCRIPT \
	"%s\n\
		{\n\
			cull none\n\
			deformVertexes wave 100 sin 1.5 0 0 0\n\
			{\n\
				map textures/sfx/construction.tga\n\
				blendfunc GL_SRC_ALPHA GL_ONE\n\
				rgbgen entity\n\
				tcGen environment\n\
				tcmod rotate 30\n\
				tcmod scroll 1 .1\n\
			}\n\
		}\n"

#define SHADER_PLAIN_SCRIPT \
	"%s\n\
		{\n\
			nomipmaps\n\
			nofog\n\
			nopicmip\n\
			{\n\
				map *white\n\
				rgbGen const ( 0 0 0 )\n\
				blendFunc gl_dst_color gl_zero\n\
			}\n\
			{\n\
				map *white\n\
				rgbGen entity\n\
				depthWrite\n\
			}\n\
		}\n"

#define SHADER_QUAD_SCRIPT \
	"%s\n\
		{\n\
			deformVertexes wave 100 sin 3 0 0 0\n\
			{\n\
				map gfx/effects/quad.tga\n\
				blendfunc GL_ONE GL_ONE\n\
				rgbgen entity\n\
				tcGen environment\n\
				depthWrite\n\
				tcmod rotate 30\n\
				tcmod scroll 1 .1\n\
			}\n\
		}\n"

#define SHADER_CRYSTAL_SCRIPT \
	"%s\n\
		{\n\
			cull none\n\
			deformVertexes wave 100 sin 2 0 0 0\n\
			noPicmip\n\
			surfaceparm trans\n\
			{\n\
				map textures/sfx/construction.tga\n\
				blendFunc GL_SRC_ALPHA GL_ONE\n\
				rgbGen entity\n\
				tcGen environment\n\
				tcMod scroll 0.025 -0.07625\n\
			}\n\
		}\n"

#define SHADER_PLASTIC_SCRIPT \
	"%s\n\
		{\n\
			deformVertexes wave 100 sin 0 0 0 0\n\
			{\n\
				map gfx/effects/fx_white.tga\n\
				rgbGen entity\n\
				blendfunc GL_ONE GL_ONE\n\
			}\n\
		}\n"

#define SHADER_CIRCLE_SCRIPT \
	"%s\n\
		{\n\
			polygonOffset\n\
			{\n\
				map gfx/effects/disk.tga\n\
				blendFunc GL_SRC_ALPHA GL_ONE\n\
				rgbGen exactVertex\n\
			}\n\
		}"
