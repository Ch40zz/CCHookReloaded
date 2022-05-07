#pragma once

struct SMedia
{
	fontInfo_t limboFont1, limboFont2;
	qhandle_t hitSounds[4], dmgSounds[4];
	qhandle_t pickupModels[11];
	qhandle_t grassModels[3];
	qhandle_t weatherSprites[3];
	qhandle_t whiteShader, coverShader, plainShader, quadShader, plasticShader,
		onFireShader, railCoreShader, reticleShader, binocShader, nullShader,
		smokepuffShader, circleShader;
	qhandle_t landmineIcon, dynamiteIcon, smokeIcon, grenadeIcon, pineappleIcon, 
		satchelIcon, medkitIcon, ammoIcon, mp40Icon, thompsonIcon, stenIcon, fg42Icon;
	qhandle_t cursorIcon, checkboxChecked, checkboxUnchecked;
};
inline SMedia media;

inline int cg_time;
inline int cg_frametime;
inline int sv_frametime;
inline int cg_showGameView;

inline refdef_t cg_refdef;
inline glconfig_t cg_glconfig;
inline snapshot_t cg_snapshot;
inline gamestate_t cgs_gameState;
inline refEntity_t cg_entities[MAX_ENTITIES];
inline bool cg_missiles[MAX_ENTITIES];
inline int cgs_levelStartTime = 0;
inline int cgs_aReinfOffset[TEAM_NUM_TEAMS];
inline bool cg_iszoomed = false;
inline int cgDC_cursorx = 320;
inline int cgDC_cursory = 240;


inline CL_CgameSystemCalls_t orig_CL_CgameSystemCalls;

inline bool showMenu = false;
inline EMod currentMod = EMod::None;
inline SClientInfo cgs_clientinfo[MAX_CLIENTS];
