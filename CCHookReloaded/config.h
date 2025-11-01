#pragma once

struct SConfig
{
	// General
	const char *pakName = "cch";


	// Chams
	bool pickupChams = true;
	vec4_t pickupVisRGBA = { 0, 255, 0, 0 };
	vec4_t pickupInvRGBA = { 0,  40, 0, 0 };

	bool missileChams = true;
	vec4_t missileVisRGBA = { 255, 0, 0, 0 };
	vec4_t missileInvRGBA = {  80, 0, 0, 0 };

	bool playerChams = true;
	bool playerOutlineChams = false;
	bool playerCorpseChams = true;
	vec4_t playerVulnRGBA   = { 255, 0, 0,   0 };
	vec4_t playerInvulnRGBA = { 255, 0, 255, 0 };
	qhandle_t& playerShader = media.coverShader;


	// Visuals
	bool scopedWalk = true;
	bool noScopeFov = false;
	bool noScopeBlackout = true;
	bool bulletTracers = false;
	bool grenadeTrajectory = true;
	bool noDamageFeedback = true;
	bool noCamExplosionShake = true;
	bool noSmoke = true;
	bool noFoliage = true;
	bool noWeather = true;


	// Aimbot
	bool aimbotEnabled = true;
	int  aimbotAimkey = VK_LBUTTON; // 0 = No Aimkey
	bool aimbotStickyAim = true;
	bool aimbotStickyAutoReset = true;
	bool aimbotLockViewangles = true;
	bool aimbotAutoshoot = false;
	bool aimbotVelocityPrediction = false;
	bool aimbotPingPrediction = false;
	bool aimbotHumanAim = false;
	float aimbotHumanFovX = 10.0f;
	float aimbotHumanFovY = 15.0f;
	float aimbotHumanFovMaxX = 5.0f;
	float aimbotHumanFovMaxY = 10.0f;
	float aimbotHumanSpeed = 0.0666f;
	float aimbotHeadBoxTraceStep = 0.5f;
	float aimbotBodyBoxTraceStep = 0.3f;
	float aimbotHeadHeightOffset = 0.0f; // -5.0f aims roughly at the neck


	// Spoofer
	// nullptr:	do not spoof
	// string:	spoof to that value, fill '?' with random hex chars
	const char *etproGuid = "????????????????????????????????????????"; //SHA1 hash, uppercase
	const char *nitmodMac = "04-92-26-??-??-??"; // MAC Address, uppercase, byte separation: '-'
	uint32_t spoofSeed = 0; // 0 = Random each game start


	// Misc
	bool spectatorWarning = true;
	bool enemySpawnTimer = true;
	bool customDmgSounds = true;
	bool quickUnbanReconnect = true; // F10
	bool cleanScreenshots = true;
	bool cvarUnlocker = false; // Might be detected by some Anti-Cheats
	bool picmipHack = false; // Visible on Screenshots
	bool bunnyHop = true;


	// ESP
	bool visibleCheck = false;
	bool headBbox = true;
	bool bodyBbox = false;
	bool boneEsp = true;
	bool nameEsp = false;
	int nameEspMode = 0; // 0 = Anchor Feet, 1 = Anchor Head
	bool missileEsp = true;
	bool missileRadius = false;
	bool pickupEsp = true;
	float maxEspDistance = FLT_MAX;
};

inline SConfig cfg;
