#pragma once

namespace eng
{
	void CG_ParseReinforcementTimes(const char *pszReinfSeedString);
	int CG_CalculateReinfTime(team_t team);
	void CG_RailTrail(const vec3_t from, const vec3_t to, const vec4_t col, int renderfx=0);
	void CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);
	bool IsPointVisible(const vec3_t start, const vec3_t pt);
	bool IsBoxVisible(const vec3_t start, const vec3_t mins, const vec3_t maxs, float step, vec3_t visOut);
	bool AimAtTarget(const vec3_t target);
	bool IsKeyActionActive(const char *action);
	hitbox_t GetHeadHitbox(const SClientInfo &ci);
	bool IsEntityArmed(const entityState_t* entState);
	bool IsValidTeam(int team);
}
