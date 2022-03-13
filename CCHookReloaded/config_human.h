#pragma once

struct SConfig
{
	// General
	bool spoofVmMainRetAddress = true;
	bool cleanScreenshots = true;
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
	bool aimbotHumanAim = true;
	bool aimbotLockViewangles = false;
	bool aimbotAutoshoot = false;


	// Spoofer
	// nullptr:	do not spoof
	// string:	spoof to that value, fill '?' with random hex chars
	const char *etproGuid = "????????????????????????????????????????"; //"9A10474B2C6BB8FFD9A770C0A926B6B7E79834C0"; //SHA1 hash, uppercase
	const char *nitmodMac = "00-20-18-??-??-??"; //"00-40-3C-76-1C-F2"; // MAC Address, uppercase, byte separation: '-'
	uint32_t spoofSeed = 0; // 0 = Random each game start


	// Misc
	bool spectatorWarning = true;
	bool enemySpawnTimer = true;
	bool customDmgSounds = true;
	bool quickUnbanReconnect = true; // F10


	// ESP
	bool headBbox = true;
	bool bodyBbox = false;
	bool boneEsp = true;
	bool nameEsp = false;
	bool missileEsp = true;
	bool pickupEsp = true;
};

inline const SConfig cfg;
