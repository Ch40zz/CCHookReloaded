#include "pch.h"
#include "globals.h"
#include "engine.h"
#include "tools.h"

#include "config.h"
#include "offsets.h"

namespace eng
{
	void CG_ParseReinforcementTimes(const char *pszReinfSeedString)
	{
		const char *tmp = pszReinfSeedString, *tmp2;
		unsigned int i, j, dwDummy, dwOffset[TEAM_NUM_TEAMS];

		#define GETVAL(x,y) if((tmp = strchr(tmp, ' ')) == NULL) return; x = atoi(++tmp)/y;

		dwOffset[TEAM_ALLIES] = atoi(pszReinfSeedString) >> REINF_BLUEDELT;
		GETVAL(dwOffset[TEAM_AXIS], (1 << REINF_REDDELT));
		tmp2 = tmp;

		for(i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
		{
			tmp = tmp2;

			for(j = 0; j<MAX_REINFSEEDS; j++)
			{
				if(j == dwOffset[i])
				{
					GETVAL(cgs_aReinfOffset[i], aReinfSeeds[j]);
					cgs_aReinfOffset[i] *= 1000;
					break;
				}

				GETVAL(dwDummy, 1);
			}
		}

		#undef GETVAL
	}
	int CG_CalculateReinfTime(team_t team)
	{
		vmCvar_t redlimbotime, bluelimbotime;
		DoSyscall(CG_CVAR_REGISTER, &redlimbotime, XorString("g_redlimbotime"), XorString("30000"), 0);
		DoSyscall(CG_CVAR_REGISTER, &bluelimbotime, XorString("g_bluelimbotime"), XorString("30000"), 0);

		int dwDeployTime = (team == TEAM_AXIS) ? redlimbotime.integer : bluelimbotime.integer;
		if (dwDeployTime == 0)
			return 0;

		return (1 + (dwDeployTime - ((cgs_aReinfOffset[team] + cg_time - cgs_levelStartTime) % dwDeployTime)) * 0.001f);
	}
	void CG_RailTrail(const vec3_t from, const vec3_t to, const vec4_t col, int renderfx)
	{
		refEntity_t ent = {};
		VectorCopy(from, ent.origin);
		VectorCopy(to, ent.oldorigin);

		ent.reType = RT_RAIL_CORE;
		ent.customShader = media.railCoreShader;

		ent.shaderRGBA[0] = col[0] * 255;
		ent.shaderRGBA[1] = col[1] * 255;
		ent.shaderRGBA[2] = col[2] * 255;
		ent.shaderRGBA[3] = col[3] * 255;

		ent.renderfx = RF_NOSHADOW | renderfx;

		DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);
	}
	void CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask)
	{
		int cg_numSolidEntities = 0,
			cg_numSolidFTEntities = 0,
			cg_numTriggerEntities = 0;

		const entityState_t *cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT],
			*cg_solidFTEntities[MAX_ENTITIES_IN_SNAPSHOT],
			*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

		auto CG_BuildSolidList = [&]() -> void {
			const size_t numEntities = std::min<size_t>(cg_snapshot.numEntities, MAX_ENTITIES_IN_SNAPSHOT);
			for (size_t i = 0 ; i < numEntities; i++)
			{
				const entityState_t *ent = &cg_snapshot.entities[i];

				if(ent->solid == SOLID_BMODEL && (ent->eFlags & EF_NONSOLID_BMODEL)) 
					continue;

				if (ent->eType == ET_ITEM || 
					ent->eType == ET_PUSH_TRIGGER || 
					ent->eType == ET_TELEPORT_TRIGGER || 
					ent->eType == ET_CONCUSSIVE_TRIGGER || 
					ent->eType == ET_OID_TRIGGER 
		#ifdef VISIBLE_TRIGGERS
					|| ent->eType == ET_TRIGGER_MULTIPLE
					|| ent->eType == ET_TRIGGER_FLAGONLY
					|| ent->eType == ET_TRIGGER_FLAGONLY_MULTIPLE
		#endif
					)
				{

					cg_triggerEntities[cg_numTriggerEntities] = ent;
					cg_numTriggerEntities++;
					continue;
				}

				if(ent->eType == ET_CONSTRUCTIBLE)
				{
					cg_triggerEntities[cg_numTriggerEntities] = ent;
					cg_numTriggerEntities++;
				}

				if (ent->solid)
				{
					cg_solidEntities[cg_numSolidEntities] = ent;
					cg_numSolidEntities++;

					cg_solidFTEntities[cg_numSolidFTEntities] = ent;
					cg_numSolidFTEntities++;
				}
			}
		};
		auto CG_ClipMoveToEntities = [&](const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask, int capsule, trace_t *tr) -> void {
			trace_t trace;
			vec3_t origin, angles;

			for (int i = 0; i < cg_numSolidEntities; i++)
			{
				const entityState_t *ent = cg_solidEntities[i];

				if (ent->number == skipNumber)
					continue;

				clipHandle_t cmodel;
				if (ent->solid == SOLID_BMODEL)
				{
					cmodel = DoSyscall(CG_CM_INLINEMODEL, ent->modelindex);

					VectorCopy(ent->angles, angles);
					VectorCopy(ent->origin, origin);
				}
				else
				{
					int x = (ent->solid & 255);
					int zd = ((ent->solid >> 8) & 255);
					int zu = ((ent->solid >> 16) & 255) - 32;

					vec3_t bmins, bmaxs;
					bmins[0] = bmins[1] = -x;
					bmaxs[0] = bmaxs[1] = x;
					bmins[2] = -zd;
					bmaxs[2] = zu;
					cmodel = DoSyscall(CG_CM_TEMPBOXMODEL, bmins, bmaxs);

					VectorCopy(vec3_origin, angles);
					VectorCopy(ent->origin, origin);
				}

				DoSyscall(CG_CM_TRANSFORMEDBOXTRACE, &trace, start, end, mins, maxs, cmodel,  mask, origin, angles);

				if (trace.allsolid || trace.fraction < tr->fraction)
				{
					trace.entityNum = ent->number;
					*tr = trace;
				}
				else if (trace.startsolid)
				{
					tr->startsolid = qtrue;
				}

				if (tr->allsolid)
					return;
			}
		};


		CG_BuildSolidList();

		DoSyscall(CG_CM_BOXTRACE, result, start, end, mins, maxs, 0, mask);

		result->entityNum = result->fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;

		CG_ClipMoveToEntities(start, mins, maxs, end, skipNumber, mask, qfalse, result);
	}
	bool IsPointVisible(const vec3_t start, const vec3_t pt)
	{
		trace_t t;
		eng::CG_Trace(&t, start, NULL, NULL, pt, cg_snapshot.ps.clientNum, MASK_SHOT);

		return (t.fraction == 1.f);
	}
	bool IsBoxVisible(const vec3_t start, const vec3_t mins, const vec3_t maxs, float step, vec3_t visOut)
	{
		// Trivial case: Middle is visible

		VectorAdd(mins, maxs, visOut);
		VectorScale(visOut, 0.5f, visOut);

		if (IsPointVisible(start, visOut))
			return true;


		// Middle wasn't visible, trace the whole box.
		// Interpolate between different box sizes up to 99%.
		// Always start with the smallest box and the middle of each edge.

		const vec3_t boxSize = 
		{
			abs(maxs[0] - mins[0]),
			abs(maxs[1] - mins[1]),
			abs(maxs[2] - mins[2])
		};

		for (float sd = step; sd < 0.99f; sd += step)
		{
			vec3_t scaledMins, scaledMaxs;
			VectorScale(boxSize, -sd/2.0f, scaledMins);
			VectorScale(boxSize, +sd/2.0f, scaledMaxs);
			VectorAdd(scaledMins, visOut, scaledMins);
			VectorAdd(scaledMaxs, visOut, scaledMaxs);

			const vec3_t boxCorner[] =
			{
				{ scaledMaxs[0], scaledMaxs[1], scaledMaxs[2] },
				{ scaledMaxs[0], scaledMaxs[1], scaledMins[2] },
				{ scaledMins[0], scaledMaxs[1], scaledMins[2] },
				{ scaledMins[0], scaledMaxs[1], scaledMaxs[2] },
				{ scaledMaxs[0], scaledMins[1], scaledMaxs[2] },
				{ scaledMaxs[0], scaledMins[1], scaledMins[2] },
				{ scaledMins[0], scaledMins[1], scaledMins[2] },
				{ scaledMins[0], scaledMins[1], scaledMaxs[2] }
			};
		
			// Try all mid values first
			for (size_t i = 0; i < std::size(boxCorner) - 1; i++)
			{
				vec3_t mid;
				VectorAdd(boxCorner[i], boxCorner[i + 1], mid);
				VectorScale(mid, 0.5f, mid);

				if (IsPointVisible(start, mid))
				{
					VectorCopy(mid, visOut);
					return true;
				}
			}

			// Try all corners last
			for (size_t i = 0; i < std::size(boxCorner); i++)
			{
				if (IsPointVisible(start, boxCorner[i]))
				{
					VectorCopy(boxCorner[i], visOut);
					return true;
				}
			}
		}

		return false;
	}
	bool AimAtTarget(const vec3_t target)
	{
		auto &localClient = cgs_clientinfo[cg_snapshot.ps.clientNum];

		vec3_t predictVieworg;
		VectorMA(cg_refdef.vieworg, cg_frametime / 1000.0f, localClient.velocity, predictVieworg);

		vec3_t dir, ang;
		VectorSubtract(target, predictVieworg, dir);
		vectoangles(dir, ang);

		vec3_t refdefViewAngles;
		vectoangles(cg_refdef.viewaxis[0], refdefViewAngles);

		float yawOffset = AngleNormalize180(ang[YAW] - refdefViewAngles[YAW]);
		float pitchOffset = AngleNormalize180(ang[PITCH] - refdefViewAngles[PITCH]);

		vec_t *angles = off::cur.viewangles();

		if (cfg.aimbotHumanAim)
		{
			const float targetDist = VectorDistance(predictVieworg, target);

			float targetScreenSizeDegX = cfg.aimbotHumanFovX / targetDist * 180.0f;
			float targetScreenSizeDegY = cfg.aimbotHumanFovY / targetDist * 180.0f;

			targetScreenSizeDegX = min(targetScreenSizeDegX, cfg.aimbotHumanFovMaxX);
			targetScreenSizeDegY = min(targetScreenSizeDegY, cfg.aimbotHumanFovMaxY);

			if (abs(yawOffset)   < targetScreenSizeDegX && 
				abs(pitchOffset) < targetScreenSizeDegY)
			{
				angles[YAW] += yawOffset * cfg.aimbotHumanSpeed;
				angles[PITCH] += pitchOffset * cfg.aimbotHumanSpeed;

				return true;
			}
		}
		else
		{
			angles[YAW] += yawOffset;
			angles[PITCH] += pitchOffset;

			return true;
		}

		return false;
	}
	bool IsKeyActionActive(const char *action)
	{
		int key1, key2;
		DoSyscall(CG_KEY_BINDINGTOKEYS, action, &key1, &key2);
	
		return DoSyscall(CG_KEY_ISDOWN, key1) || DoSyscall(CG_KEY_ISDOWN, key2);
	}
	hitbox_t GetHeadHitbox(const SClientInfo &ci)
	{
		const int modIndex = static_cast<int>(currentMod);
		auto &hitbox = head_hitboxes[modIndex];

		if (ci.flags & (EF_PRONE | EF_PRONE_MOVING))
			return { hitbox.prone_offset, hitbox.size };

		bool isMoving = !!VectorCompare(ci.velocity, vec3_origin);

		if (ci.flags & EF_CROUCHING)
			return { isMoving ? hitbox.crouch_offset_moving : hitbox.crouch_offset, hitbox.size };

		return { isMoving ? hitbox.stand_offset_moving : hitbox.stand_offset, hitbox.size };
	}
	bool IsEntityArmed(const entityState_t* entState)
	{
		if (currentMod == EMod::Legacy)
			return entState->effect1Time != 0;

		return entState->teamNum == TEAM_AXIS || entState->teamNum == TEAM_ALLIES;
	}
	bool IsValidTeam(int team)
	{
		return team == TEAM_AXIS || team == TEAM_ALLIES;
	}
	qhandle_t RegisterAndLoadShader(const char *shaderData, uint32_t seed)
	{
		char shaderName[17] = {};
		tools::RandomizeHexString(shaderName, std::size(shaderName) - 1, '\0', seed);

		char newShaderData[1024];
		if (sprintf_s(newShaderData, shaderData, shaderName) < 0)
			return 0;

		(void)DoSyscall(CG_R_LOADDYNAMICSHADER, shaderName, newShaderData);
		return DoSyscall(CG_R_REGISTERSHADER, shaderName);
	}
}
