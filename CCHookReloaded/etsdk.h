#pragma once

#include "ETSDK/src/cgame/cg_local.h"
#include "ETSDK/src/ui/ui_public.h"
#include "ETSDK/src/ui/ui_shared.h"


// Custom cheat types/structs

enum class EMod
{
	None,
	EtMain,
	EtPub,
	EtPro,
	JayMod,
	Nitmod,
	NoQuarter,
	Silent,
	Legacy,
	Unknown
};

struct SClientInfo
{
	bool valid;

	int id;
	bool invuln;
	int flags;
	int weapId;

	// Sent via snapshot (not interpolated, possibly extrapolated)
	vec3_t origin;
	vec3_t velocity;

	// Updated and interpolated by client
	vec3_t interOrigin;

	int teamNum;
	int classNum;
	char name[MAX_QPATH];
	char cleanName[MAX_QPATH];
};

struct SDraw3dCommand
{
	enum class EType
	{
		RailTrail,
		Decal,
	} type;

	union
	{
		struct
		{
			vec3_t from;
			vec3_t to;
			vec4_t col;
			int renderfx;
		} railTrail;

		struct
		{
			vec3_t pos;
			vec3_t dir;
			vec4_t col;
			qhandle_t shader;
			float scale;
		} decal;
	};

	SDraw3dCommand() = default;

	SDraw3dCommand(const vec3_t from, const vec3_t to, const vec4_t col, int renderfx)
	{
		type = EType::RailTrail;

		VectorCopy(from, railTrail.from);
		VectorCopy(to, railTrail.to);
		Vector4Copy(col, railTrail.col);
		railTrail.renderfx = renderfx;
	}

	SDraw3dCommand(const vec3_t pos, const vec3_t dir, const vec4_t col, qhandle_t shader, float scale)
	{
		type = EType::Decal;

		VectorCopy(pos, decal.pos);
		VectorCopy(dir, decal.dir);
		Vector4Copy(col, decal.col);
		decal.shader = shader;
		decal.scale = scale;
	}
};
