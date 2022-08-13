/* Disclaimer:
	This cheat has been successfully tested on several mods including:
	- ETLegacy
	- EtMain
	- EtPub
	- EtPro (+Spoofer)
	- JayMod
	- Nitmod (+Spoofer)
	- NoQuarter
	- Silent

	While testing the cheat, several mods turned out to have their own
	Anti-Cheat and detecting simple things e.g. modified entities on
	CG_R_ADDREFENTITYTOSCENE or certain hardcoded strings in memory
	not backed by any module.
	Every Anti-Cheat has been reverse engineered and bypassed, including
	simple HWID generation of ETPro and Nitmod to recognize/ban players.
	ETPro is using various serial numbers from the hard drive(s), Nitmod
	is using the MAC Address of the first network card in the system.

	There is however no guarentee that this cheat stays undetected,
	so please assume the cheat is detected until proven otherwise
	to avoid unneccessary etkey and HWID bans on your real account/hardware.
*/

#include "pch.h"
#include "globals.h"
#include "tools.h"
#include "engine.h"
#include "ui.h"

#include "config.h"
#include "offsets.h"

// Bit set if weapon is a hitscan weapon (enables aimbot)
// Warning: This is mod dependent!
constexpr uint64_t hitscanWeaponBitmap = 0xFFFF9FE78380458C;

vm_t hookVM;
uintptr_t hookTimer = 0;
volatile bool disableRendering = true;
bool lockViewangles = false;
uint32_t spoofSeed;

int old_reliableSequence = 0;
bool etpro_set_rollingkey = false;
uint8_t etpro_rollingkey[8];

std::vector<SDraw3dCommand> draw3dCommands;


EMod InitializeMod()
{
	char modName[MAX_PATH+1];
	EMod mod = tools::GetETMod(modName);
	printf(XorString("Detected mod: %s (%i)\n"), modName, mod);

	// Unlock PAK before we load all of its media
	tools::UnlockPak();


	// Disable annoying camera bob
	DoSyscall(CG_CVAR_SET, XorString("cg_bobPitch"), XorString("0"));
	DoSyscall(CG_CVAR_SET, XorString("cg_bobYaw"), XorString("0"));
	DoSyscall(CG_CVAR_SET, XorString("cg_bobRoll"), XorString("0"));
	DoSyscall(CG_CVAR_SET, XorString("cg_bobUp"), XorString("0"));

	DoSyscall(CG_GETGLCONFIG, &cg_glconfig);
	printf(XorString("Renderer initialized with resolution %ix%i\n"), cg_glconfig.vidWidth, cg_glconfig.vidHeight);

	DoSyscall(CG_R_REGISTERFONT, XorString("ariblk"), 27, &media.limboFont1);
	DoSyscall(CG_R_REGISTERFONT, XorString("courbd"), 30, &media.limboFont2);

	for (size_t i = 0; i < std::size(media.hitSounds); i++)
	{
		char soundName[256];
		sprintf_s(soundName, XorString("sound/weapons/impact/metal%i.wav"), i + 1);
		media.hitSounds[i] = DoSyscall(CG_S_REGISTERSOUND, soundName, qfalse);
	}

	for (size_t i = 0; i < std::size(media.dmgSounds); i++)
	{
		char soundName[256];
		sprintf_s(soundName, XorString("sound/weapons/impact/flesh%i.wav"), i + 1);
		media.dmgSounds[i] = DoSyscall(CG_S_REGISTERSOUND, soundName, qfalse);
	}

	media.pickupModels[0] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/multiplayer/ammopack/ammopack_pickup.md3"));
	media.pickupModels[1] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/multiplayer/medpack/medpack_pickup.md3"));
	media.pickupModels[2] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/multiplayer/supplies/healthbox_wm.md3"));
	media.pickupModels[3] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/multiplayer/supplies/ammobox_wm.md3"));
	media.pickupModels[4] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/mapobjects/supplystands/stand_health.md3"));
	media.pickupModels[5] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/mapobjects/supplystands/stand_ammo.md3"));
	media.pickupModels[6] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/weapons2/mp40/mp40.md3"));
	media.pickupModels[7] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/weapons2/thompson/thompson.md3"));
	media.pickupModels[8] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/weapons2/sten/sten.md3"));
	media.pickupModels[9] =  DoSyscall(CG_R_REGISTERMODEL, XorString("models/weapons2/fg42/fg42.md3"));
	media.pickupModels[10] = DoSyscall(CG_R_REGISTERMODEL, XorString("models/multiplayer/mauser/mauser_pickup.md3"));

	XorS(coverShaderText, "%s\n\
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
		}\n");

	XorS(plainShaderText, "%s\n\
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
		}\n");

	XorS(quadShaderText, "%s\n\
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
		}\n");

	XorS(crystalShaderText, "%s\n\
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
		}\n");

	XorS(plasticShaderText, "%s\n\
		{\n\
			deformVertexes wave 100 sin 0 0 0 0\n\
			{\n\
				map gfx/effects/fx_white.tga\n\
				rgbGen entity\n\
				blendfunc GL_ONE GL_ONE\n\
			}\n\
		}\n");

	XorS(circleShaderText, "%s\n\
		{\n\
			polygonOffset\n\
			{\n\
				map gfx/effects/disk.tga\n\
				blendFunc GL_SRC_ALPHA GL_ONE\n\
				rgbGen exactVertex\n\
			}\n\
		}");

	media.coverShader = eng::RegisterAndLoadShader(coverShaderText.decrypt(), spoofSeed+0);
	media.plainShader = eng::RegisterAndLoadShader(plainShaderText.decrypt(), spoofSeed+1);
	media.quadShader = eng::RegisterAndLoadShader(quadShaderText.decrypt(), spoofSeed+2);
	media.plasticShader = eng::RegisterAndLoadShader(plasticShaderText.decrypt(), spoofSeed+3);
	media.circleShader = eng::RegisterAndLoadShader(circleShaderText.decrypt(), spoofSeed+4);

	media.railCoreShader = DoSyscall(CG_R_REGISTERSHADERNOMIP, XorString("railCore"));
	media.onFireShader = DoSyscall(CG_R_REGISTERSHADERNOMIP, XorString("entityOnFire1"));
	media.reticleShader = DoSyscall(CG_R_REGISTERSHADERNOMIP, XorString("gfx/misc/reticlesimple.tga"));
	media.binocShader = DoSyscall(CG_R_REGISTERSHADERNOMIP, XorString("gfx/misc/binocsimple.tga"));
	media.nullShader = DoSyscall(CG_R_REGISTERSHADERNOMIP, XorString("gfx/2d/nullshader.tga"));
	media.smokepuffShader = DoSyscall(CG_R_REGISTERSHADERNOMIP, XorString("smokePuff"));
	media.whiteShader = DoSyscall(CG_R_REGISTERSHADER, XorString("white"));

	media.landmineIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_landmine_1_select.tga"));
	media.dynamiteIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_dynamite_1_select.tga"));
	media.smokeIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_smokegrenade_1_select.tga"));
	media.grenadeIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_grenade_1_select.tga"));
	media.pineappleIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_pineapple_1_select.tga"));
	media.satchelIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_satchel_1_select.tga"));
	media.medkitIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_medheal_select.tga"));
	media.ammoIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_ammopack_1_select.tga"));
	media.mp40Icon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_MP40_1_select.tga"));
	media.thompsonIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_thompson_1_select.tga"));
	media.stenIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_sten_1_select.tga"));
	media.fg42Icon = DoSyscall(CG_R_REGISTERSHADER, XorString("icons/iconw_fg42_1_select.tga"));
	media.cursorIcon = DoSyscall(CG_R_REGISTERSHADER, XorString("ui/assets/3_cursor3.tga"));
	media.checkboxChecked = DoSyscall(CG_R_REGISTERSHADER, XorString("ui/assets/check.tga"));
	media.checkboxUnchecked = DoSyscall(CG_R_REGISTERSHADER, XorString("ui/assets/check_not.tga"));


	// Ensure "pak->referenced" is set back to 0 again after PAK was used
	tools::UnlockPak();

	return mod;
}
void PatchLoadedImages(bool enabled)
{
	static bool lastState = false;
	static std::unordered_map<std::string, unsigned int> origTrImages;
	static unsigned int lastEmptyTexture = 0;

	if (lastState == enabled)
		return;

	lastState = enabled;


	const int numImages = off::cur.tr_numImages();
	image_t **images = off::cur.tr_images();

	if (enabled)
	{
		origTrImages.clear();

		unsigned int emptyTexture = 0;
		for (size_t i = 0; i < numImages; i++)
		{
			if (strstr(images[i]->imgName, XorString("gfx/2d/nullshader.tga")))
				emptyTexture = images[i]->texnum;
		}

		for (size_t i = 0; i < numImages; i++)
		{
			if (cfg.noFoliage)
			{
				if (strstr(images[i]->imgName, XorString("models/foliage")) ||
					strstr(images[i]->imgName, XorString("models/mapobjects/plants")) ||
					strstr(images[i]->imgName, XorString("models/mapobjects/tree")))
				{
					origTrImages[images[i]->imgName] = images[i]->texnum;
					images[i]->texnum = emptyTexture;
				}
			}

			if (cfg.noWeather)
			{
				if (strstr(images[i]->imgName, XorString("gfx/misc/rain")) ||
					strstr(images[i]->imgName, XorString("gfx/misc/snow")))
				{
					origTrImages[images[i]->imgName] = images[i]->texnum;
					images[i]->texnum = emptyTexture;
				}
			}
		}

		lastEmptyTexture = emptyTexture;
	}
	else
	{
		// Restore default textures

		for (size_t i = 0; i < numImages; i++)
		{
			const auto &it = origTrImages.find(images[i]->imgName);
			if (it != origTrImages.cend() && images[i]->texnum == lastEmptyTexture)
				images[i]->texnum = it->second;
		}

		origTrImages.clear();
	}
}
void etpro_SpoofGUID(const char *newGuid)
{
	constexpr int BUFFER_SIZE = 1024;
	const int reliableSequence = off::cur.clc_reliableSequence();

	if (old_reliableSequence != reliableSequence)
	{
		for (int seq = old_reliableSequence + 1 ; seq <= reliableSequence; seq++)
		{
			char *command = off::cur.reliableCommands() + BUFFER_SIZE * (seq & (MAX_RELIABLE_COMMANDS - 1));

			if (!memcmp(command, XorString("notice "), 7))
			{
				// ETPro command to notify the server of a cheat detection.
				// "notice <PayloadBase64>"
				//
				// We do not really need to block this detection but why not while we're at it

				printf(XorString("WARNING: BLOCKED POTENTIAL NOTICE DETECTION: '%s'\n"), command);

				strncpy(command, XorString("score"), BUFFER_SIZE);
			}
			else if (!memcmp(command, XorString("C!"), 2) || !memcmp(command, XorString("E!"), 2))
			{
				// ETPro command to transmit collected detection data to the server.
				// "[C!|E!]<Sha1Base64> <PayloadBase64>"

				// As long as the rolling key has not been set yet, we can't spoof the commands.
				// Therefore we replace the custom ETPro commands with simple "score" commands.
				// The server will re-transmit a new rollingkey and the client will resend these lost commands.
				if (!etpro_set_rollingkey)
				{
					strncpy(command, XorString("score"), BUFFER_SIZE);
					continue;
				}

				// Step 1: Parse the command and check hashes

				char* payload_b64 = strstr(command, " ") + 1;

				char sha1_b64[40];
				int sha1_b64_len = payload_b64 - (command + 2) - 1;
				memcpy(sha1_b64, command + 2, sha1_b64_len);
				sha1_b64[sha1_b64_len] = '\0';

				char sha1_embedded[40];
				Base64decode(sha1_embedded, sha1_b64);

				char sha1[20];
				SHA1_CTX ctx;
				SHA1Init(&ctx);
				SHA1Update(&ctx, etpro_rollingkey, sizeof(etpro_rollingkey));
				SHA1Update(&ctx, (uint8_t*)payload_b64, strlen(payload_b64));
				SHA1Update(&ctx, etpro_sha1_salt, sizeof(etpro_sha1_salt));
				SHA1Final((uint8_t*)sha1, &ctx);

				if (memcmp(sha1_embedded, sha1, sizeof(sha1)))
				{
					// Should never really happen when the rolling key has been set.
					// Just to play safe we assert this here and drop the ETPro commands to get a new rollingkey.

					printf(XorString("WARNING: HASHES DO NOT MATCH; SKIPPING COMMAND '%s'\n"), command);

					strncpy(command, XorString("score"), BUFFER_SIZE);
					continue;
				}


				// Step 2: Extract the payload and modify it if needed

				char payload_data[BUFFER_SIZE];
				int payload_len = Base64decode(payload_data, payload_b64);

				uint32_t crc32_hash = 0;
				
				// First 3 bytes are random numbers which are not encrypted but used as seed for the hash
				for (size_t i = 0; i < 3; i++)
					crc32_hash = crc32_tab[(uint8_t)(crc32_hash ^ payload_data[i])] ^ (crc32_hash >> 8);

				char str_clc_challenge[16];
				sprintf_s(str_clc_challenge, XorString("%i"), off::cur.clc_challenge());

				for(size_t i = 0; str_clc_challenge[i]; i++)
					crc32_hash = crc32_tab[(uint8_t)(crc32_hash ^ str_clc_challenge[i])] ^ (crc32_hash >> 8);

				uint32_t crc32_init_hash = crc32_hash;

				// Decrypt payload
				for (size_t i = 3; i < payload_len; i++)
				{
					payload_data[i] ^= crc32_hash >> 8;
					crc32_hash = crc32_tab[(uint8_t)(crc32_hash ^ payload_data[i])] ^ (crc32_hash >> 8);
				}

				// Parse and edit payload
				if (command[0] == 'C')
				{
					/*etpro_payload_C_t *payload = (etpro_payload_C_t*)payload_data;

					printf("module_hash: %X\n", payload->module_hash);
					printf("module_detection_flags: %X\n", payload->module_detection_flags);
					printf("module_info: %s\n", payload->module_info);*/
				}
				else if (command[0] == 'E')
				{
					etpro_payload_E_t *payload = (etpro_payload_E_t*)payload_data;

					/*printf("detection_flags: %X\n", payload->detection_flags);
					printf("et_hash: %X\n", payload->et_hash);
					printf("cgame_hash: %X\n", payload->cgame_hash);*/

					if (payload->detection_flags & (~0x3FFF))
					{
						printf(XorString("WARNING: BLOCKED POTENTIAL DETECTION-FLAGS DETECTION: %X\n"), payload->detection_flags);

						// We remove any potential detections (there are none though, cheat is fully undetected)
						payload->detection_flags &= 0x3FFF;
					}

					printf(XorString("original etproguid: %s\n"), payload->etproguid);

					memcpy(payload->etproguid, newGuid, sizeof(payload->etproguid));
					tools::RandomizeHexString(payload->etproguid, sizeof(payload->etproguid), '?', spoofSeed);

					printf(XorString(" spoofed etproguid: %s\n"), payload->etproguid);
				}
				else
				{
					printf(XorString("WARNING: UNKNOWN MESSAGE TYPE; CANNOT SPOOF INFORMATION\n"));

					strncpy(command, XorString("score"), BUFFER_SIZE);
				}

				crc32_hash = crc32_init_hash;

				// Re-encrypt payload
				for (size_t i = 3; i < payload_len; i++)
				{
					uint8_t prev = crc32_hash ^ payload_data[i];
					payload_data[i] ^= crc32_hash >> 8;
					crc32_hash = crc32_tab[prev] ^ (crc32_hash >> 8);
				}


				// Step 3: Repackage the data into a send-ready command
				
				char payload_encoded[BUFFER_SIZE];
				int payload_enc_len = Base64encode(payload_encoded, payload_data, payload_len);

				SHA1Init(&ctx);
				SHA1Update(&ctx, etpro_rollingkey, sizeof(etpro_rollingkey));
				SHA1Update(&ctx, (uint8_t*)payload_encoded, payload_enc_len - 1);
				SHA1Update(&ctx, etpro_sha1_salt, sizeof(etpro_sha1_salt));
				SHA1Final((uint8_t*)sha1, &ctx);

				for (size_t i = 0; i < sizeof(etpro_rollingkey); i++)
					etpro_rollingkey[i] ^= sha1[i];

				Base64encode(sha1_b64, sha1, sizeof(sha1));

				strcpy(command + 2, sha1_b64);
				strcat(command, XorString(" "));
				strcat(command, payload_encoded);
			}
			else if(!memcmp(command, XorString("userinfo "), 9))
			{
				// Default command to auth to server.
				// Used by nitmod to send HWID data (MAC Address & GUID).
				// The GUID is a hash (MD5) of the MAC Address and other data.
				// I was too lazy to reverse the whole code because there is a much easier way to bypass this check:
				// One can simply overwrite the "X" CVAR and the AC will happily calculate the new correct hash for us :)
				
				/*for (char *macAddress = command; macAddress = tools::Info_ValueForKeyInbuff(macAddress, "x"); )
				{
					memcpy(macAddress, cfg.nitmodMac, 17);
					tools::RandomizeHexString(macAddress, 17, '?', ~spoofSeed);
				}

				for (char *nitGuid = command; nitGuid = tools::Info_ValueForKeyInbuff(nitGuid, "n_guid"); )
				{
					memcpy(nitGuid, cfg.nitmodGuid, 32);
					tools::RandomizeHexString(nitGuid, 32, '?', spoofSeed*0x01000193);
				}*/
			}
			else
			{
				//printf("reliableCommand: %s\n", command);
			}
		}

		old_reliableSequence = reliableSequence;
	}
}
void nitmod_SpoofMAC(const char *newMac)
{
	char tmpMac[18];
	memcpy(tmpMac, newMac, sizeof(tmpMac));
	tools::RandomizeHexString(tmpMac, sizeof(tmpMac), '?', ~spoofSeed);

	// This is pretty memey:
	// The "x" CVar contains the MAC Address read by Nitmod.
	// It is used to craft the hardware GUID that is used to recognize and ban players.
	// By modifying this variable we don't need to reverse and spoof the packet sent to server.
	DoSyscall(CG_CVAR_REGISTER, 0, XorString("x"), tmpMac, sizeof(tmpMac));
	DoSyscall(CG_CVAR_SET, XorString("x"), tmpMac);
}


intptr_t hooked_CL_CgameSystemCalls(intptr_t *args)
{
	switch (args[0])
	{
	case CG_S_UPDATEENTITYPOSITION:
	{
		int entityNum = args[1];
		float *origin = (float*)args[2];

		if (entityNum < 0 || entityNum >= MAX_ENTITIES)
			break;
		
		if (entityNum < MAX_CLIENTS)
			VectorCopy(origin, cgs_clientinfo[entityNum].interOrigin);

		cg_entities[entityNum].reType = RT_MODEL;
		VectorCopy(origin, cg_entities[entityNum].origin);
		break;
	}
	case CG_R_ADDREFENTITYTOSCENE:
	{
		// We copy the entity to make sure we do not change the data of the real entity.
		// Mainly detected by nxAC (nitmod)
		refEntity_t ent = *(refEntity_t*)args[1];
		if (ent.entityNum < 0 || ent.entityNum >= MAX_ENTITIES)
			break;

		cg_entities[ent.entityNum] = ent;

		if (disableRendering)
			break;
		if (ent.reType != RT_MODEL)
			break;

		if (cfg.pickupChams)
		{
			// For pickup items, entityNum is always the playerID. Check hModel instead.

			for (size_t i = 0; i < std::size(media.pickupModels); i++)
			{
				if (ent.hModel != media.pickupModels[i])
					continue;

				const bool isVisible = eng::IsPointVisible(cg_refdef.vieworg, ent.origin);

				ent.renderfx |=  isVisible ? RF_DEPTHHACK : 0;
				DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);

				ent.customShader = media.coverShader;
				Vector4Copy(isVisible ? cfg.pickupVisRGBA : cfg.pickupInvRGBA, ent.shaderRGBA);
				ent.renderfx |=  RF_DEPTHHACK | RF_NOSHADOW;
				if (isVisible) ent.renderfx &= ~RF_DEPTHHACK;

				return DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);
			}
		}

		if (cfg.missileChams)
		{
			if (cg_missiles[ent.entityNum])
			{
				const bool isVisible = eng::IsPointVisible(cg_refdef.vieworg, ent.origin);

				ent.renderfx |= isVisible ? RF_DEPTHHACK : 0;
				DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);

				ent.customShader = media.coverShader;
				Vector4Copy(isVisible ? cfg.missileVisRGBA : cfg.missileInvRGBA, ent.shaderRGBA);
				ent.renderfx |= RF_DEPTHHACK | RF_NOSHADOW;
				if (isVisible) ent.renderfx &= ~RF_DEPTHHACK;

				return DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);
			}
		}

		if (cfg.playerChams)
		{
			if (!(ent.renderfx & RF_LIGHTING_ORIGIN) ||
				ent.renderfx & (RF_DEPTHHACK | RF_NOSHADOW))
				break;
			if (!ent.customSkin)
				break;
			if (ent.entityNum < 0 || ent.entityNum >= MAX_CLIENTS)
				break;

			auto& ci = cgs_clientinfo[ent.entityNum];
			if (!ci.valid)
				break;
			if (ci.teamNum == cgs_clientinfo[cg_snapshot.ps.clientNum].teamNum)
				break;

			if (ci.flags & EF_DEAD)
			{
				if (cfg.playerCorpseChams)
				{
					DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);

					ent.shaderRGBA[3] = 255;
					ent.customShader = media.onFireShader;
				}
			}
			else
			{
				if (cfg.playerOutlineChams)
				{
					ent.renderfx |= RF_DEPTHHACK | RF_NOSHADOW;
					DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);

					ent.renderfx &= ~(RF_DEPTHHACK | RF_NOSHADOW);
					ent.customShader = media.coverShader;
					Vector4Copy(ci.invuln ? cfg.playerInvulnRGBA : cfg.playerVulnRGBA, ent.shaderRGBA);
				}

				DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);

				ent.customShader = media.coverShader;
				Vector4Copy(ci.invuln ? cfg.playerInvulnRGBA : cfg.playerVulnRGBA, ent.shaderRGBA);
			}
		}
		
		return DoSyscall(CG_R_ADDREFENTITYTOSCENE, &ent);
	}
	case CG_R_ADDPOLYTOSCENE:
	{
		const qhandle_t hShader = args[1];
		const int numVerts = args[2];
		const polyVert_t *verts = (polyVert_t*)args[3];

		if (disableRendering)
			break;

		// No Smoke: Modify smoke alpha to make it see through
		if (cfg.noSmoke)
		{
			if (hShader == media.smokepuffShader && numVerts == 4)
			{
				polyVert_t newVerts[4];
				memcpy(&newVerts, verts, sizeof(newVerts));

				// Reduce alpha by 80%
				for (size_t i = 0; i < std::size(newVerts); i++)
					newVerts[i].modulate[3] /= 5;

				return DoSyscall(CG_R_ADDPOLYTOSCENE, hShader, numVerts, newVerts);
			}
		}

		break;
	}
	case CG_R_RENDERSCENE:
	{
		refdef_t *refdef = (refdef_t*)args[1];

		// Make sure it is the main camera/refdef
		if(refdef->x > 1 || refdef->y > 1)
			break;

		// Restore the default FoV (undo scope zooming)
		if (cfg.noScopeFov && cg_iszoomed && !disableRendering)
		{
			char fov[64] = { };
			DoSyscall(CG_CVAR_VARIABLESTRINGBUFFER, XorString("cg_fov"), fov, sizeof(fov));
			refdef->fov_x = fov[0] ? atof(fov) : 90.0f;

			float x = refdef->width / tan(refdef->fov_x / 360 * M_PI);
			refdef->fov_y = atan2(refdef->height, x);
			refdef->fov_y *= 360 / M_PI;
		}

		memcpy(&cg_refdef, refdef, sizeof(cg_refdef));

		if (!disableRendering)
		{
			// Rail Trails for local player shots
			if (cfg.bulletTracers)
			{
				static int oldWeaponTime = INT_MIN;
				static vec3_t oldVieworg, oldViewaxis;
				static int lifetime = 0;

				if (cg_snapshot.ps.weaponstate == WEAPON_FIRING && 
					(cg_snapshot.ps.ammoclip[cg_snapshot.ps.weapon] > 0 || cg_iszoomed) &&
					cg_snapshot.ps.weaponTime >= 40)
				{
					if (cg_snapshot.ps.weaponTime > oldWeaponTime)
					{
						VectorCopy(cg_refdef.vieworg, oldVieworg);
						VectorCopy(cg_refdef.viewaxis[0], oldViewaxis);
						lifetime = 200;
					}

					oldWeaponTime = cg_snapshot.ps.weaponTime;
				}

				if (lifetime > 0)
				{
					lifetime -= cg_frametime;
			
					vec3_t from, to;
					from[0] = oldVieworg[0];
					from[1] = oldVieworg[1];
					from[2] = oldVieworg[2] - 10.0f;

					VectorMA(oldVieworg, 8192, oldViewaxis, to);

					eng::CG_RailTrail(from, to, colorRed);
				}
			}

			// Process custom 3D draw commands from vmMain - they can only be drawn in CG_RenderScene
			for (SDraw3dCommand cmd : draw3dCommands)
			{
				switch (cmd.type)
				{
				case SDraw3dCommand::EType::RailTrail:
				{
					eng::CG_RailTrail(cmd.railTrail.from, cmd.railTrail.to, cmd.railTrail.col, cmd.railTrail.renderfx);
					break;
				}
				case SDraw3dCommand::EType::Decal:
				{
					vec4_t projection;
					VectorSubtract(vec3_origin, cmd.decal.dir, projection);
					projection[3] = cmd.decal.scale;

					vec4_t markOrigin;
					VectorMA(cmd.decal.pos, -16.0f, projection, markOrigin);
					
					DoSyscall(CG_R_PROJECTDECAL, cmd.decal.shader, 1, markOrigin, projection, cmd.decal.col, cg_frametime, cg_frametime*16);
					break;
				}
				}
			}

			draw3dCommands.clear();
		}

		cg_iszoomed = false;

		break;
	}
	case CG_R_DRAWSTRETCHPIC:
	{
		if (disableRendering)
			break;

		float x = *(float*)&args[1];
		float y = *(float*)&args[2];
		float w = *(float*)&args[3];
		float h = *(float*)&args[4];
		qhandle_t shader = args[9];

		if (cfg.noScopeBlackout)
		{
			// Remove sniper/binocular cutout image
			if (shader == media.reticleShader || shader == media.binocShader)
			{
				cg_iszoomed = true;
				return 0;
			}

			// Remove blackout area when zooming
			if (shader == media.whiteShader && y + h == cg_glconfig.vidHeight &&
				(x == 0 && y == 0 || x + w == cg_glconfig.vidWidth))
			{
				cg_iszoomed = true;
				return 0;
			}
		}

		break;
	}
	case CG_SETUSERCMDVALUE:
	{
		int userCmdValue = args[1]; // cg.weaponSelect -> cmd->weapon
		int flags = args[2]; // cg.showGameView -> cmd->flags
		float sensitivityScale = *(float*)&args[3]; // cg.zoomSensitivity -> mouseSense
		int mpIdentClient = args[4]; // cg.identifyClientRequest -> entityNum

		// 1 if limbo menu is open
		cg_showGameView = !!(flags & 1);

		// Remove any possible nitmod detection.
		// This is not really needed because the cheat is undetected anyways, but why not?
		if (currentMod == EMod::Nitmod)
		{
			if (flags & ~1)
				printf(XorString("WARNING: NITMOD CHEAT DETECTED, BYPASSING DETECTION; FLAGS: %X\n"), flags);

			flags &= 1;
		}

		// Prevent low sensitivity when fov is restored
		if (cfg.noScopeFov && sensitivityScale != 0.0f)
			sensitivityScale = 1.0f;

		// Lock view angles
		if (cfg.aimbotLockViewangles && lockViewangles)
			sensitivityScale = 0.0f;

		return DoSyscall(CG_SETUSERCMDVALUE, userCmdValue, flags, sensitivityScale, mpIdentClient);
	}
	case CG_GETSNAPSHOT:
	{
		intptr_t success = orig_CL_CgameSystemCalls(args);
		if (success)
		{
			//intptr_t snapshotNumber = args[1];
			snapshot_t *snapshot = (snapshot_t*)args[2];

			sv_frametime = snapshot->serverTime - cg_snapshot.serverTime;
			memcpy(&cg_snapshot, snapshot, sizeof(cg_snapshot));

			// No damage feedback on bullet impacts
			if (cfg.noDamageFeedback)
				snapshot->ps.damageEvent = 0;

			// Avoid zoomed sniper from switching weapon when moving too fast
			if (cfg.scopedWalk && cg_iszoomed)
				VectorClear(snapshot->ps.velocity);

			memset(cg_missiles, 0, sizeof(cg_missiles));

			const size_t numEntities = std::min<size_t>(snapshot->numEntities, MAX_ENTITIES_IN_SNAPSHOT);
			for (size_t i = 0; i < numEntities; i++)
			{
				auto& ent = snapshot->entities[i];
				if (ent.number < 0 || ent.number >= MAX_ENTITIES)
					continue;

				// Dirty hack to fix event types on ET:Legacy, probably want to use the full changed struct instead
				const auto Type = (currentMod == EMod::Legacy && ent.eType > EV_EMPTYCLIP) 
					? ent.eType+1
					: ent.eType;

				if (Type == ET_EVENTS + EV_SHAKE)
				{
					// No camera shake on explosions
					if (cfg.noCamExplosionShake)
						ent.eType = ET_INVISIBLE;
				}
				else if (Type == ET_MISSILE && ent.number >= MAX_CLIENTS)
				{
					// Track missiles for ESP/Chams
					cg_missiles[ent.number] = true;
				}

				// Technically we could check for "Type == ET_PLAYER".
				// Gibbed players however get their entity type changed to "ET_INVISIBLE".
				// Since we know all numbers below "MAX_CLIENTS" are players, just don't filter.
				if (ent.number < MAX_CLIENTS)
				{
					// Track player data for ESP/Aimbot/...

					auto &ci = cgs_clientinfo[ent.number];

					ci.valid = true;
					ci.invuln = !!(ent.powerups & (1 << PW_INVULNERABLE));
					ci.flags = ent.eFlags;
					ci.weapId = ent.weapon;

					VectorCopy(ent.pos.trBase, ci.origin);
					VectorCopy(ent.pos.trDelta, ci.velocity);
				}
			}
		}
		
		return success;
	}
	case CG_GETGAMESTATE:
	{
		intptr_t ret = orig_CL_CgameSystemCalls(args);

		gameState_t *gameState = (gameState_t*)args[1];

		int gameStateOffset = gameState->stringOffsets[CS_WOLFINFO];
		if (gameStateOffset)
			cgs_gameState = (gamestate_t)atoi(Info_ValueForKey(gameState->stringData + gameStateOffset, XorString("gamestate")));

		int reinfOffset = gameState->stringOffsets[CS_REINFSEEDS];
		if(reinfOffset)
			eng::CG_ParseReinforcementTimes(gameState->stringData + reinfOffset);

		int lvlStartTimeOffset = gameState->stringOffsets[CS_LEVEL_START_TIME];
		if(lvlStartTimeOffset)
			cgs_levelStartTime = atoi(gameState->stringData + lvlStartTimeOffset);

		if (gameState->stringOffsets[CS_PLAYERS + cg_snapshot.ps.clientNum])
		{
			// Make sure we only reset all clientInfo data when the gamestate contains new clientInfos

			for(size_t i = 0; i < MAX_CLIENTS; i++) 
			{
				auto &ci = cgs_clientinfo[i];

				bool wasValid = ci.valid;
				ci.valid = false;

				int playerOffset = gameState->stringOffsets[CS_PLAYERS + i];
				if (!playerOffset)
					continue;

				// Player just connected, initialize the struct
				if (!wasValid)
					memset(&ci, 0, sizeof(ci));

				ci.valid = true;
				ci.id = i;

				ci.teamNum = atoi(Info_ValueForKey(gameState->stringData + playerOffset, XorString("t")));
				ci.classNum = atoi(Info_ValueForKey(gameState->stringData + playerOffset, XorString("c")));
				strncpy(ci.name, Info_ValueForKey(gameState->stringData + playerOffset, XorString("n")), sizeof(ci.name)-1);
				strncpy(ci.cleanName, ci.name, sizeof(ci.cleanName)-1);
				Q_CleanStr(ci.cleanName);
			}
		}

		return ret;
	}
	case CG_GETSERVERCOMMAND:
	{
		// In order to fake our ETPro GUID we need to emulate the "csum" packet responses.
		// The protocol uses a rolling key which is initialized by the server and is arg[1] of the command.

		intptr_t success = orig_CL_CgameSystemCalls(args);
		if (!success)
			return success;

		int argc = DoSyscall(CG_ARGC);

		/*for (int i = 0; i < argc; i++)
		{
			char buff[64];
			DoSyscall(CG_ARGV, i, buff, sizeof(buff));
			printf("%s ", buff);
		}
		printf("\n");*/

		// Get command name
		char buff[64];
		DoSyscall(CG_ARGV, 0, buff, sizeof(buff));

		if (argc == 2 && !_stricmp(buff, XorString("csum")))
		{
			// Get RollingKey initializer
			DoSyscall(CG_ARGV, 1, buff, sizeof(buff));
			if (strlen(buff) == 11)
			{
				Base64decode((char*)etpro_rollingkey, buff);

				char str_clc_challenge[16];
				sprintf_s(str_clc_challenge, XorString("%i"), off::cur.clc_challenge());

				for (size_t i = 0; i < sizeof(etpro_rollingkey); i++)
					etpro_rollingkey[i] ^= str_clc_challenge[i];

				etpro_set_rollingkey = true;
			}
		}

		return success;
	}
	case CG_GETUSERCMD:
	{
		intptr_t success = orig_CL_CgameSystemCalls(args);
		if (!success)
			return success;

		//usercmd_t *cmd = (usercmd_t*)args[2];

		return success;
	}
	}

	return orig_CL_CgameSystemCalls(args);
}

typedef void (__thiscall *glReadPixels_t)(void *_this, int x, int y, int width, int height, int format, int type, void *pixels);
glReadPixels_t orig_glReadPixels;
void __fastcall hooked_glReadPixels(void *_this, void *edx, int x, int y, int width, int height, int format, int type, void *pixels)
{
	printf(XorString("glReadPixels: Screenshot requested! Blocking...\n"));

	disableRendering = true;

	off::cur.SCR_UpdateScreen()();
	off::cur.SCR_UpdateScreen()();
	off::cur.SCR_UpdateScreen()();
	off::cur.SCR_UpdateScreen()();
	off::cur.SCR_UpdateScreen()();

	orig_glReadPixels(_this, x, y, width, height, format, type, pixels);

	disableRendering = false;
}

typedef intptr_t (__cdecl *vmMain_t)(intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, 
	intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, 
	intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16);
vmMain_t orig_vmMain;
intptr_t __cdecl hooked_vmMain(intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, 
	intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11,
	intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16)
{
	if (hookTimer)
	{
		KillTimer(nullptr, hookTimer);
		hookTimer = 0;
	}

	// Hook syscall func by replacing currentVM
	vm_t *curVM = off::cur.currentVM();
	memcpy(&hookVM, (void*)curVM, sizeof(hookVM));
	orig_CL_CgameSystemCalls = hookVM.systemCall;
	hookVM.systemCall = hooked_CL_CgameSystemCalls;
	off::cur.currentVM() = &hookVM;

	auto VmMainCall = [&](intptr_t _id, intptr_t _a1=0, intptr_t _a2=0, intptr_t _a3=0, intptr_t _a4=0, intptr_t _a5=0, 
							intptr_t _a6=0, intptr_t _a7=0, intptr_t _a8=0, intptr_t _a9=0, intptr_t _a10=0, intptr_t _a11=0,
							intptr_t _a12=0, intptr_t _a13=0, intptr_t _a14=0, intptr_t _a15=0, intptr_t _a16=0) -> intptr_t {
		// Restore vmMain original temporarily
		vm_t *_cgvm = off::cur.cgvm();
		_cgvm->entryPoint = orig_vmMain;

		// Spoof the return address when calling "orig_vmMain" to make sure ACs don't easily detect our hook (e.g. as ETPro does).
		// NOTE: This is not supported for ET:Legacy but support can be added if required.
		intptr_t result;
		switch (off::cur.VmSpoofType())
		{
		case off::COffsets::EVmSpoofType::Call12:
			result = SpoofCall12(off::cur.VM_Call_vmMain(), (uintptr_t)orig_vmMain, _id, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8, _a9, _a10, _a11, _a12, 0, 0, 0);
			break;
		case off::COffsets::EVmSpoofType::Call12_Steam:
			result = SpoofCall12_Steam(off::cur.VM_Call_vmMain(), (uintptr_t)orig_vmMain, _id, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8, _a9, _a10, _a11, _a12, 0, 0, 0, 0);
			break;
		default:
			result = orig_vmMain(_id, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8, _a9, _a10, _a11, _a12, _a13, _a14, _a15, _a16);
			break;
		}

		// Rehook vmMain
		_cgvm->entryPoint = hooked_vmMain;


		// Restore currentVM
		off::cur.currentVM() = curVM;

		return result;
	};
	
	intptr_t result = VmMainCall(id, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16);

	if (id == CG_MOUSE_EVENT)
	{
		const int dx = a1;
		const int dy = a2;

		if (showMenu)
		{
			cgDC_cursorx += dx;
			if (cgDC_cursorx < 0) cgDC_cursorx = 0;
			if (cgDC_cursorx > 640) cgDC_cursorx = 640;

			cgDC_cursory += dy;
			if (cgDC_cursory < 0) cgDC_cursory = 0;
			if (cgDC_cursory > 480) cgDC_cursory = 480;
		}
	}
	else if (id == CG_DRAW_ACTIVE_FRAME)
	{
		const int serverTime = a1;

		cg_frametime = serverTime - cg_time;
		cg_time = serverTime;

		if (currentMod == EMod::None)
		{
			// Detect mod, register/lookup needed media only once
			currentMod = InitializeMod();

			// Reset patched images back to original
			PatchLoadedImages(false);

			old_reliableSequence = off::cur.clc_reliableSequence();
		}

		if (currentMod == EMod::EtPro &&  cfg.etproGuid && cfg.etproGuid[0]) etpro_SpoofGUID(cfg.etproGuid);
		if (currentMod == EMod::Nitmod && cfg.nitmodMac && cfg.nitmodMac[0]) nitmod_SpoofMAC(cfg.nitmodMac);

#ifdef USE_DEBUG
		if (GetAsyncKeyState(VK_F9)&1)
			disableRendering = !disableRendering;
#endif

		PatchLoadedImages(!disableRendering);


		// If limbo menu is open or screenshot is in progress do not draw ESP
		if (cg_showGameView || disableRendering)
			return result;

		auto &localClient = cgs_clientinfo[cg_snapshot.ps.clientNum];

		// Spectator Warning
		if (cfg.spectatorWarning)
		{
			std::vector<std::string> spectatorNames;
			if (tools::GatherSpectatorInfo(spectatorNames) && !spectatorNames.empty())
			{
				ui::DrawText(320.0f, 90.0f, 0.20f, 0.20f, colorRed, XorString("WARNING - Spectator:"),
					0.0f, 0, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, &media.limboFont1);

				for (size_t i = 0; i < spectatorNames.size(); i++)
					ui::DrawText(320.0f, 100.0f + i*10.0f, 0.20f, 0.20f, colorWhite, spectatorNames[i].c_str(), 
						0.0f, 0, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, &media.limboFont1);
			}
		}

		// Enemy Spawn Timer
		if (cfg.enemySpawnTimer)
		{
			if (eng::IsValidTeam(localClient.teamNum))
			{
				team_t enemyTeam = team_t(localClient.teamNum ^ 0b11);

				char enemyReinfTime[64];
				sprintf_s(enemyReinfTime, XorString("%i"), eng::CG_CalculateReinfTime(enemyTeam));
				ui::DrawBoxedText(630.0f, 137.0f, 0.18f, 0.18f, colorRed, enemyReinfTime, 
					0.0f, 0, ITEM_TEXTSTYLE_NORMAL, ITEM_ALIGN_RIGHT, &media.limboFont1, colorMenuBg, colorMenuBo);
			}
		}

		// Enemy Hit Sounds, Local Damage Sounds
		if (cfg.customDmgSounds)
		{
			static int totalDamage = 0;
			if (totalDamage != cg_snapshot.ps.persistant[PERS_HITS])
			{
				// Make sure we hit an enemy
				if (cg_snapshot.ps.persistant[PERS_HITS] - totalDamage > 0)
					DoSyscall(CG_S_STARTLOCALSOUND, media.hitSounds[serverTime % std::size(media.hitSounds)], CHAN_LOCAL_SOUND, 100);

				totalDamage = cg_snapshot.ps.persistant[PERS_HITS];
			}
			
			static int lastHealth = 0;
			if (cg_snapshot.ps.pm_type == PM_NORMAL && !(cg_snapshot.ps.eFlags & EF_DEAD) &&
				lastHealth != cg_snapshot.ps.stats[STAT_HEALTH])
			{
				if (cg_snapshot.ps.stats[STAT_HEALTH] < lastHealth)
					DoSyscall(CG_S_STARTLOCALSOUND, media.dmgSounds[serverTime % std::size(media.dmgSounds)], CHAN_LOCAL_SOUND, 100);

				lastHealth = cg_snapshot.ps.stats[STAT_HEALTH];
			}
			else
			{
				lastHealth = 0;
			}
		}

		// Player ESP
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			auto &ci = cgs_clientinfo[i];

			if (!ci.valid)
				continue;
			if (i == localClient.id)
				continue;
			if (!eng::IsValidTeam(ci.teamNum))
				continue;
			if (ci.teamNum == localClient.teamNum)
				continue;
			if (ci.flags & EF_DEAD)
				continue;
			if (VectorDistance(cg_refdef.vieworg, ci.interOrigin) > cfg.maxEspDistance)
				continue;
			orientation_t orTmp;
			if (!VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_head")), intptr_t(&orTmp)))
				continue;


			// Head Bounding Box ESP
			if (cfg.headBbox)
			{
				orientation_t orHead;
				if (VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_head")), intptr_t(&orHead)))
				{
					const auto &hitbox = eng::GetHeadHitbox(ci);

					vec3_t mins, maxs;
					VectorScale(hitbox.size, -0.5f, mins);
					VectorScale(hitbox.size, +0.5f, maxs);

					vec3_t offset;
					VectorCopy(orHead.origin, offset);
					VectorMA(offset, hitbox.offset[0], orHead.axis[0], offset);
					VectorMA(offset, hitbox.offset[1], orHead.axis[1], offset);
					VectorMA(offset, hitbox.offset[2], orHead.axis[2], offset);

					VectorAdd(mins, offset, mins);
					VectorAdd(maxs, offset, maxs);

					ui::DrawBox3D(mins, maxs, 1.0f, colorGreen);
				}
			}


			// Body Bounding Box ESP
			if (cfg.bodyBbox)
			{
				vec3_t mins = {-18, -18, -24};
				vec3_t maxs = { 18,  18,  48};

				VectorAdd(mins, ci.interOrigin, mins);
				VectorAdd(maxs, ci.interOrigin, maxs);

				ui::DrawBox3D(mins, maxs, 1.0f, colorGreen);
			}


			// Bone ESP
			if (cfg.boneEsp)
			{
				orientation_t orHead, orChest, orArmL, orArmR, orHandL, orHandR, orTorso, orHipsL, orHipsR, orLegL, orLegR, orFootL, orFootR;
				if (VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_head")), intptr_t(&orHead)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_chest")), intptr_t(&orChest)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_armleft")), intptr_t(&orArmL)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_armright")), intptr_t(&orArmR)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_weapon2")), intptr_t(&orHandL)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_weapon")), intptr_t(&orHandR)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_torso")), intptr_t(&orTorso)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_bleft")), intptr_t(&orHipsL)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_bright")), intptr_t(&orHipsR)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_legleft")), intptr_t(&orLegL)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_legright")), intptr_t(&orLegR)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_footleft")), intptr_t(&orFootL)) &&
					VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_footright")), intptr_t(&orFootR)))
				{
					ui::DrawLine3D(orHead.origin, orChest.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orChest.origin, orArmL.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orChest.origin, orArmR.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orArmL.origin, orHandL.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orArmR.origin, orHandR.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orChest.origin, orTorso.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orTorso.origin, orHipsL.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orTorso.origin, orHipsR.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orHipsL.origin, orLegL.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orHipsR.origin, orLegR.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orLegL.origin, orFootL.origin, 2.0f, colorGreen);
					ui::DrawLine3D(orLegR.origin, orFootR.origin, 2.0f, colorGreen);
				}
			}


			// Name ESP
			if (cfg.nameEsp)
			{
				if (cfg.nameEspMode == 0)
				{
					vec3_t feetAnchor;
					VectorCopy(ci.interOrigin, feetAnchor);
					feetAnchor[2] -= 30.0f;

					float x, y;
					if (ui::WorldToScreen(feetAnchor, &x, &y))
						ui::DrawText(x, y, 0.14f, 0.14f, colorWhite, ci.name, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, &media.limboFont1);
				}
				else
				{
					orientation_t orHead;
					if (VmMainCall(CG_GET_TAG, i, intptr_t(XorString("tag_head")), intptr_t(&orHead)))
					{
						orHead.origin[2] += 15.0f;

						float x, y;
						if (ui::WorldToScreen(orHead.origin, &x, &y))
							ui::DrawText(x, y - 8.0f, 0.14f, 0.14f, colorWhite, ci.name, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, &media.limboFont1);
					}
				}
			}
		}

		// Missile & Pickup ESP
		const size_t numEntities = std::min<size_t>(cg_snapshot.numEntities, MAX_ENTITIES_IN_SNAPSHOT);
		for (size_t i = 0; i < numEntities; i++)
		{
			auto& ent = cg_snapshot.entities[i];
			if (ent.number < 0 || ent.number >= MAX_ENTITIES)
				continue;

			const auto& cgEnt = cg_entities[ent.number];
			if (cgEnt.reType == RT_MAX_REF_ENTITY_TYPE)
				continue;
			if (VectorDistance(cg_refdef.vieworg, cgEnt.origin) > cfg.maxEspDistance)
				continue;

			if (cfg.missileEsp && ent.eType == ET_MISSILE)
			{
				float x, y;
				if (!ui::WorldToScreen(cgEnt.origin, &x, &y))
					continue;

				if (ent.weapon == WP_LANDMINE && eng::IsEntityArmed(&ent))
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorRed, media.landmineIcon);
				else if (ent.weapon == WP_DYNAMITE)
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorRed, media.dynamiteIcon);
				else if (ent.weapon == WP_SMOKE_BOMB)
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorRed, media.smokeIcon);
				else if (ent.weapon == WP_GRENADE_PINEAPPLE)
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorRed, media.pineappleIcon);
				else if (ent.weapon == WP_GRENADE_LAUNCHER)
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorRed, media.grenadeIcon);
				else if (ent.weapon == WP_SATCHEL)
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorRed, media.satchelIcon);

				// Draw detonation timer for dynamite
				if (ent.weapon == WP_DYNAMITE && eng::IsEntityArmed(&ent))
				{
					const int time = 30 - (cg_time - ent.effect1Time) / 1000;

					char timeString[64];
					sprintf_s(timeString, XorString("%i"), time);
					ui::DrawText(x, y + 10.0f, 0.12f, 0.12f, colorRed, timeString, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, &media.limboFont1);

					if (cfg.missileRadius && time <= 5)
					{
						const vec3_t dir = { 0, 0, -1 };
						draw3dCommands.emplace_back(cgEnt.origin, dir, colorRed, media.circleShader, 420.0f);
					}
				}
				else if(cfg.missileRadius && (ent.weapon == WP_GRENADE_LAUNCHER || ent.weapon == WP_GRENADE_PINEAPPLE) && ent.time)
				{
					const vec3_t dir = { 0, 0, -1 };
					draw3dCommands.emplace_back(cgEnt.origin, dir, colorRed, media.circleShader, 260.0f);
				}
			}
			else if(cfg.pickupEsp && ent.eType == ET_ITEM)
			{
				float x, y;
				if (!ui::WorldToScreen(cgEnt.origin, &x, &y))
					continue;

				// TODO: Do not hardcode model indices.
				// Pickup items use the playerID for entityNum in ADDREFENTITYTOSCENE instead.
				if (ent.modelindex == 3)
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorGreen, media.medkitIcon);
				else if (ent.modelindex == 33)
					ui::DrawIcon(x, y, 15.0f, 15.0f, colorGreen, media.ammoIcon);
				else if (ent.modelindex == 19)
					ui::DrawIcon(x, y, 30.0f, 15.0f, colorGreen, media.mp40Icon);
				else if (ent.modelindex == 13)
					ui::DrawIcon(x, y, 30.0f, 15.0f, colorGreen, media.thompsonIcon);
				else if (ent.modelindex == 15)
					ui::DrawIcon(x, y, 30.0f, 15.0f, colorGreen, media.stenIcon);
				else if (ent.modelindex == 44)
					ui::DrawIcon(x, y, 30.0f, 15.0f, colorGreen, media.fg42Icon);
			}
		}


		// Aimbot
		if (cfg.aimbotEnabled)
		{
			lockViewangles = false;

			static int aimTargetId = -1;
			bool enableAimbot = (hitscanWeaponBitmap & (1ull << cg_snapshot.ps.weapon)) && 
				cfg.aimbotAimkey == 0 ? true : (GetKeyState(cfg.aimbotAimkey) & 0x8000);

			if (!enableAimbot)
			{
				aimTargetId = -1;
			}
			else
			{
				auto PredictAimPos = [](SClientInfo &ci, vec3_t aimPos) -> void {
					// Velocity prediction 1 frame ahead
					//VectorSubtract(aimPos, ci.interOrigin, aimPos);
					//VectorAdd(aimPos, ci.origin, aimPos);

					if (cfg.aimbotAntiLagCompensation)
						VectorMA(aimPos, -(cg_frametime + cg_snapshot.ping/2) / 1000.0f, ci.velocity, aimPos);
				};
				auto GetAimPos = [&](SClientInfo &ci, vec3_t aimPos, bool traceBox, bool traceHead) -> bool {
					if (!ci.valid)
						return false;
					if (ci.id == localClient.id)
						return false;
					if (!eng::IsValidTeam(ci.teamNum))
						return false;
					if (ci.teamNum == localClient.teamNum)
						return false;
					if (ci.flags & EF_DEAD)
						return false;
					if (ci.invuln)
						return false;

					const auto &hitbox = eng::GetHeadHitbox(ci);

					orientation_t tagOrient;
					if (!VmMainCall(CG_GET_TAG, ci.id, intptr_t(XorString("tag_head")), intptr_t(&tagOrient)))
						return false;

					if (traceBox)
					{
						if (traceHead)
						{
							vec3_t mins, maxs;
							VectorScale(hitbox.size, -0.5f, mins);
							VectorScale(hitbox.size, +0.5f, maxs);

							vec3_t offset;
							VectorCopy(tagOrient.origin, offset);
							VectorMA(offset, hitbox.offset[0], tagOrient.axis[0], offset);
							VectorMA(offset, hitbox.offset[1], tagOrient.axis[1], offset);
							VectorMA(offset, hitbox.offset[2], tagOrient.axis[2], offset);

							PredictAimPos(ci, offset);

							VectorAdd(mins, offset, mins);
							VectorAdd(maxs, offset, maxs);

							// SnapVector(ent->s.pos.trBase + ent->client->ps.viewheight)
							if (eng::IsBoxVisible(cg_refdef.vieworg, mins, maxs, cfg.aimbotHeadBoxTraceStep, aimPos))
								return true;
						}
						else
						{
							vec3_t origin;
							vec3_t mins = {-18, -18, -24};
							vec3_t maxs = { 18,  18,  48};

							VectorCopy(ci.interOrigin, origin);
							PredictAimPos(ci, origin);

							VectorAdd(mins, origin, mins);
							VectorAdd(maxs, origin, maxs);

							if (eng::IsBoxVisible(cg_refdef.vieworg, mins, maxs, cfg.aimbotBodyBoxTraceStep, aimPos))
								return true;
						}
					}
					else
					{
						VectorCopy(tagOrient.origin, aimPos);

						// Get Center of hitbox
						VectorMA(aimPos, hitbox.offset[0], tagOrient.axis[0], aimPos);
						VectorMA(aimPos, hitbox.offset[1], tagOrient.axis[1], aimPos);
						VectorMA(aimPos, hitbox.offset[2], tagOrient.axis[2], aimPos);

						PredictAimPos(ci, aimPos);
					
						if (eng::IsPointVisible(cg_refdef.vieworg, aimPos))
							return true;
					}

					return false;
				};
				auto GetAimTarget = [&GetAimPos](vec3_t aimPos, bool traceBox, bool traceHead) -> int {
					int targetId = -1;
					float targetDist = 0;

					for (int i = 0; i < MAX_CLIENTS; i++)
					{
						vec3_t localAimPos;
						if (!GetAimPos(cgs_clientinfo[i], localAimPos, traceBox, traceHead))
							continue;

						float x, y;
						if (!ui::WorldToScreen(localAimPos, &x, &y))
							continue;

						float xc = cg_glconfig.vidWidth/2;
						float yc = cg_glconfig.vidHeight/2;
						float dist = sqrt((x-xc)*(x-xc) + (y-yc)*(y-yc));

						if (targetId != -1 && dist >= targetDist)
							continue;

						targetId = i;
						targetDist = dist;
						VectorCopy(localAimPos, aimPos);
					}

					return targetId;
				};

				vec3_t aimPos;
				bool isTargetValid = false;

				// Sticky aim should be disabled default when no aimkey is set
				if ((cfg.aimbotStickyAim && aimTargetId == -1) || !cfg.aimbotStickyAim || cfg.aimbotAimkey == 0)
				{
					isTargetValid = (aimTargetId = GetAimTarget(aimPos, true, true)) != -1;
					if (!isTargetValid)
						isTargetValid = (aimTargetId = GetAimTarget(aimPos, true, false)) != -1;
				}
				else
				{
					if (!(isTargetValid = GetAimPos(cgs_clientinfo[aimTargetId], aimPos, true, true)))
						isTargetValid = GetAimPos(cgs_clientinfo[aimTargetId], aimPos, true, false);

					if (!isTargetValid && cfg.aimbotStickyAutoReset)
					{
						isTargetValid = (aimTargetId = GetAimTarget(aimPos, true, true)) != -1;
						if (!isTargetValid)
							isTargetValid = (aimTargetId = GetAimTarget(aimPos, true, false)) != -1;
					}
				}

				if (isTargetValid && (lockViewangles = eng::AimAtTarget(aimPos)))
				{
					// Autoshoot
					if (cfg.aimbotAutoshoot)
					{
						//// Works too but why require another offset if we can just inject a console command?
						//kbutton_t *kb = off::cur.kbuttons();
						//kb[KB_DOWN].active = 1;
						//kb[KB_BUTTONS0].wasPressed = 1;

						DoSyscall(CG_SENDCONSOLECOMMAND, XorString("+attack\n-attack\n"));
					}
				}
			}
		}


		// Rifle and Grenade trajectory
		if (cfg.grenadeTrajectory && !cg_iszoomed)
		{
			auto SnapVectorTowards = [](vec3_t v, vec3_t to) {
				for (int i = 0 ; i < 3 ; i++)
					v[i] = (to[i] <= v[i]) ? floor(v[i]) : ceil(v[i]);
			};

			// Dirty hack to fix weapons on ET:Legacy, probably want a map of all weapons per mod
			const auto curWeapon = (currentMod == EMod::Legacy && cg_snapshot.ps.weapon > WP_TRIPMINE) 
				? cg_snapshot.ps.weapon+2 
				: cg_snapshot.ps.weapon;

			const bool IsRifleGrenade = curWeapon == WP_GPG40 || curWeapon == WP_M7;
			const bool IsThrowGrenade = curWeapon == WP_GRENADE_LAUNCHER || curWeapon == WP_GRENADE_PINEAPPLE;

			if (IsRifleGrenade || IsThrowGrenade)
			{
				// For reference code check "G_BounceMissile", "weapon_gpg40_fire" and "weapon_grenadelauncher_fire" in original ET source code

				vec3_t forward, right, up;
				AngleVectors(cg_snapshot.ps.viewangles, forward, right, up);

				vec3_t muzzlePoint;
				VectorCopy(cg_snapshot.ps.origin, muzzlePoint);
				muzzlePoint[2] += cg_snapshot.ps.viewheight;

				if (IsThrowGrenade)
				{
					VectorMA(muzzlePoint, 20, right, muzzlePoint);
				}
				else
				{
					VectorMA(muzzlePoint, 6, right, muzzlePoint);
					VectorMA(muzzlePoint, -4, up, muzzlePoint);
				}

				SnapVector(muzzlePoint);


				vec3_t tossPos;

				if (IsRifleGrenade)
				{
					vec3_t viewPos, origViewPos;
					VectorCopy(muzzlePoint, tossPos);
					VectorCopy(cg_snapshot.ps.origin, viewPos);
					viewPos[2] += cg_snapshot.ps.viewheight;
					VectorCopy(viewPos, origViewPos);
					VectorMA(viewPos, 32, forward, viewPos);

					trace_t tr;
					eng::CG_Trace(&tr, origViewPos, tv(-4.f, -4.f, 0.f), tv(4.f, 4.f, 6.f), viewPos, cg_snapshot.ps.clientNum, MASK_MISSILESHOT);
					if (tr.fraction < 1)
					{
						// Bad launch spot
						VectorCopy(tr.endpos, tossPos);
						SnapVectorTowards(tossPos, origViewPos);
					}
					else
					{
						eng::CG_Trace(&tr, viewPos, tv(-4.f, -4.f, 0.f), tv(4.f, 4.f, 6.f), tossPos, cg_snapshot.ps.clientNum, MASK_MISSILESHOT);
						if (tr.fraction < 1)
						{
							// Bad launch spot
							VectorCopy(tr.endpos, tossPos);
							SnapVectorTowards(tossPos, viewPos);
						}
					}

					VectorScale(forward, 2000.0f, forward);
				}
				else
				{
					float pitch = cg_snapshot.ps.viewangles[PITCH];
					if (pitch >= 0)
					{
						forward[2] += 0.5f;
						pitch = 1.3f;
					}
					else
					{
						pitch = -pitch;
						pitch = min(pitch, 30);
						pitch /= 30.f;
						pitch = 1 - pitch;
						forward[2] += (pitch * 0.5f);

						pitch *= 0.3f;
						pitch += 1.f;
					}

					VectorNormalizeFast(forward);

					float upangle = -cg_snapshot.ps.viewangles[PITCH];
					upangle = min(upangle, 50);
					upangle = max(upangle, -50);
					upangle = upangle / 100.0f;
					upangle += 0.5f;

					if (upangle < 0.1)
						upangle = 0.1;

					upangle *= 900 * pitch; // Only dynamite is "heavier" with a speed of 400

					VectorMA(muzzlePoint, 8, forward, tossPos);
					tossPos[2] -= 8;
					SnapVector(tossPos);

					VectorScale(forward, upangle, forward);
				}

				trajectory_t trajectory;
				trajectory.trType = TR_GRAVITY;
				trajectory.trTime = cg_time - MISSILE_PRESTEP_TIME;
				VectorCopy(tossPos, trajectory.trBase);
				VectorCopy(forward, trajectory.trDelta);
				SnapVector(trajectory.trDelta);

				vec3_t lastPos, lastPredictedPos, predictedNormal, predictedPos;
				VectorCopy(trajectory.trBase, lastPos);


				constexpr int TIME_STEP = 5;

				int lastBounce = 0;
				int groundEntityNum = ENTITYNUM_NONE;
				for (int totalTime = MISSILE_PRESTEP_TIME; totalTime < 4000; totalTime += TIME_STEP)
				{
					// Predict the next impact position for each "frame".
					// If we hit an object the missile either explodes (after 750ms+ air time)
					// or it will bounce off the surface and continue to fly.
					// If no object is hit it will simply continue flying on the same trajectory.

					const int curBounceTime = totalTime - lastBounce;
					BG_EvaluateTrajectory(&trajectory, cg_time + curBounceTime, predictedPos, qfalse, 0);

					trace_t tr;
					eng::CG_Trace(&tr, lastPos, nullptr, nullptr, predictedPos, cg_snapshot.ps.clientNum, MASK_MISSILESHOT);

					VectorSubtract(vec3_origin, tr.plane.normal, predictedNormal);
					VectorCopy(tr.endpos, lastPos);

					if ((tr.startsolid || tr.fraction != 1) && !(tr.surfaceFlags & (SURF_SKY | SURF_NOIMPACT)))
					{
						// Missile hit an object that is not the sky/noimpact

						if (IsRifleGrenade)
						{
							if (totalTime > 750)
								break;
						}

						// Calculate reflect angle
						const int hitTime = cg_time + (curBounceTime - TIME_STEP) + (tr.fraction * TIME_STEP);
						BG_EvaluateTrajectoryDelta(&trajectory, hitTime, trajectory.trDelta, qfalse, 0);
						float dot = DotProduct(trajectory.trDelta, tr.plane.normal);
						VectorMA(trajectory.trDelta, -2 * dot, tr.plane.normal, trajectory.trDelta);

						if (tr.plane.normal[2] > 0.2)
							groundEntityNum = tr.entityNum;

						VectorScale(trajectory.trDelta, 0.35, trajectory.trDelta);

						if (groundEntityNum != ENTITYNUM_WORLD)
							VectorScale(trajectory.trDelta, 0.5, trajectory.trDelta);

						SnapVector(trajectory.trDelta);

						VectorCopy(tr.endpos, trajectory.trBase);
						SnapVector(trajectory.trBase);

						// Stop the missile
						if (tr.plane.normal[2] > 0.2 && VectorLengthSquared(trajectory.trDelta) < SQR(40))
							break;

						lastBounce = totalTime;
						trajectory.trTime = cg_time;
					}

					if (totalTime > MISSILE_PRESTEP_TIME)
					{
						const vec4_t& color = eng::IsPointVisible(cg_refdef.vieworg, predictedPos) ? colorYellow : colorRed;

						//ui::DrawLine3D(lastPredictedPos, predictedPos, 2.0f, color);
						draw3dCommands.emplace_back(lastPredictedPos, predictedPos, color, RF_DEPTHHACK);
					}

					VectorCopy(predictedPos, lastPredictedPos);
				}

				// Draw circle indicating explosion radius at the final destination
				draw3dCommands.emplace_back(predictedPos, predictedNormal, colorRed, media.circleShader, 64.0f);
			}
		}

		for (size_t i = 0; i < std::size(cg_entities); i++)
			cg_entities[i].reType = RT_MAX_REF_ENTITY_TYPE;

		ui::DrawMenu();
	}

	return result;
}


bool PlaceCGHook()
{
	if (cfg.cleanScreenshots)
	{
		// Hook the internal OpenGL function-table.
		// The pointer to the table is stored per thread (Thread Local Storage).
		// This function will always be called by the main (render) thread.
		HMODULE opengl32 = GetModuleHandleA(XorString("opengl32.dll"));
		if (opengl32)
		{
			uint8_t *glReadPixels = (uint8_t*)GetProcAddress(opengl32, XorString("glReadPixels"));
			if (glReadPixels)
			{
				// Find the TLS Index that OpenGL allocated 
				DWORD TlsIndex = MAXDWORD;
				if (glReadPixels[5] == 0xA1)
				{
					// mov eax, dword ptr ds:[imm32]
					TlsIndex = **(DWORD**)&glReadPixels[6];
				}
				else if(glReadPixels[7] == 0x83 && glReadPixels[8] == 0x3D)
				{
					// cmp dword ptr ds:[imm32], imm8
					TlsIndex = **(DWORD**)&glReadPixels[9];
				}
				else
				{
					const auto *idh = (IMAGE_DOS_HEADER*)opengl32;
					const auto *inh = (IMAGE_NT_HEADERS*)((uint8_t*)opengl32 + idh->e_lfanew);

					// FF 15 B4 72 2F 69       call    ds:__imp__TlsAlloc@0 ; TlsAlloc()
					// A3 8C 89 2D 69          mov     _dwTlsIndex, eax
					// 83 F8 FF                cmp     eax, 0FFFFFFFFh
					uint8_t *pattern = tools::FindPattern((uint8_t*)opengl32 + inh->OptionalHeader.BaseOfCode, inh->OptionalHeader.SizeOfCode, 
														  (uint8_t*)XorString("\xFF\x15\x00\x00\x00\x00\xA3\x00\x00\x00\x00\x83\xF8\xFF"), XorString("xx????x????xxx"));
					if (pattern)
						TlsIndex = **(DWORD**)&pattern[7];
				}

				if (TlsIndex != MAXDWORD)
				{
					void **glCltProcTable = (void**)TlsGetValue(TlsIndex);

					if (glCltProcTable[256] != hooked_glReadPixels)
					{
						disableRendering = false;

						orig_glReadPixels = (glReadPixels_t)glCltProcTable[256];
						glCltProcTable[256] = hooked_glReadPixels;
					}
				}
			}
		}

		if (disableRendering)
			printf(XorString("WARNING: FAILED TO HOOK 'opengl32!glReadPixels', DISABLING RENDERING!\n"));
	}

	vm_t *_cgvm = off::cur.cgvm();
	if (_cgvm)
	{
		vmMain_t vmMain = _cgvm->entryPoint;
		if (vmMain && vmMain != hooked_vmMain)
		{
			currentMod = EMod::None;
			old_reliableSequence = 0;
			etpro_set_rollingkey = false;

			orig_vmMain = vmMain;
			_cgvm->entryPoint = hooked_vmMain;

			printf(XorString("Hooking vmMain...\n"));

			return true;
		}
	}

	return false;
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

typedef decltype(refexport_t::Shutdown) Shutdown_t;
Shutdown_t orig_Shutdown;
void hooked_Shutdown(qboolean destroyWindow)
{
	// This is called right before a new renderer is initialized.
	// We use it to start a timer to hook the next initialized renderer.
	// This is only needed when no new mod is loaded and we stay on the same cgame_mp_x86.dll.

	if (destroyWindow)
		hookTimer = SetTimer(nullptr, (UINT_PTR)TimerProc, USER_TIMER_MINIMUM, TimerProc);

	orig_Shutdown(destroyWindow);
}

typedef decltype(refexport_t::EndRegistration) EndRegistration_t;
EndRegistration_t orig_EndRegistration;
void hooked_EndRegistration()
{
	// This is called right after a new mod was loaded and successfully initialized.
	// It is called right after CG_INIT - we use it as event to know CG_INIT was called.

	orig_EndRegistration();

	PlaceCGHook();
}

typedef decltype(refexport_t::EndFrame) EndFrame_t;
EndFrame_t orig_EndFrame;
void hooked_EndFrame(int *frontEndMsec, int *backEndMsec)
{
	// This function is called each frame even in the main menu.
	// Here we can have functions that should execute independently of the loaded mod.

	// Simple Un-Ban and auto-reconnect key:
	// Generates a new ETKey and randomizes the net_port.
	if (cfg.quickUnbanReconnect && GetAsyncKeyState(VK_F10)&1)
	{
		// Set new spoof seed to change etpro guid and mac address on reconnect
		spoofSeed = GetTickCount() * tools::Rand();

		char newEtKey[32];
		strcpy_s(newEtKey, XorString("0000001002"));
		for (size_t i = 0; i < 18; i++)
			newEtKey[i + 10] = '0' + (tools::Rand() % 10);

		char etDir[MAX_PATH + 1], backupPath[MAX_PATH + 1];
		uint32_t len = GetModuleFileNameA(GetModuleHandleA(nullptr), etDir, sizeof(etDir));

		for(; len > 0; len--)
		{
			if (etDir[len - 1] == '\\' || etDir[len - 1] == '/')
			{
				etDir[len - 1] = '\0';
				break;
			}
		}

		strcat_s(etDir, XorString("\\etmain\\etkey"));

		// Backup the old key
		sprintf_s(backupPath, XorString("%s_%X"), etDir, GetTickCount());
		CopyFileA(etDir, backupPath, true);

		FILE *etkey = fopen(etDir, XorString("w"));
		if (etkey)
		{
			fwrite((void*)newEtKey, 1, 28, etkey);
			fclose(etkey);
		}

		char reconnectCommand[96];
		strcpy_s(reconnectCommand, XorString("pb_myguid; net_port 27???; net_restart; vid_restart; pb_myguid; reconnect"));
		for (size_t i = 0; i < 3; i++)
			reconnectCommand[i + 11] = '0' + (tools::Rand() % 10);

		HWND consoleTextbox = FindWindowExA(FindWindowA(nullptr, 
			off::cur.IsEtLegacy() ? XorString("ET: Legacy Console") : XorString("ET Console")), nullptr, XorString("Edit"), 0);

		SendMessageA(consoleTextbox, WM_SETTEXT, 0, (LPARAM)reconnectCommand);
		SendMessageA(consoleTextbox, WM_CHAR, 13, 0); // Simulate Enter
	}

	// Hooking at this stage can be detected by an Anti-Cheat (and ETPro actually does) so do not do it here
	//PlaceCGHook();

	orig_EndFrame(frontEndMsec, backEndMsec);
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	PlaceCGHook();
}

void CALLBACK LdrDllNotification(ULONG NotificationReason, LDR_DLL_NOTIFICATION_DATA *NotificationData, PVOID Context)
{
	// Called whenever a DLL is loaded or unloaded.
	// This is used to initially hook some functions in order to not require a thread.
	// Whenever a mod loads the new "cgame_mp_x86.dll" we can rehook the renderer.

	if (off::cur.IsEtLegacy())
	{
		legacy_refexport_t *re = (legacy_refexport_t*)off::cur.refExport();

		if (re->Shutdown && re->Shutdown != hooked_Shutdown)
		{
			orig_Shutdown = re->Shutdown;
			re->Shutdown = hooked_Shutdown;
		}

		if (re->EndRegistration && re->EndRegistration != hooked_EndRegistration)
		{
			orig_EndRegistration = re->EndRegistration;
			re->EndRegistration = hooked_EndRegistration;
		}

		if (re->EndFrame && re->EndFrame != hooked_EndFrame)
		{
			orig_EndFrame = re->EndFrame;
			re->EndFrame = hooked_EndFrame;
		}
	}
	else
	{
		refexport_t *re = off::cur.refExport();

		if (re->Shutdown && re->Shutdown != hooked_Shutdown)
		{
			orig_Shutdown = re->Shutdown;
			re->Shutdown = hooked_Shutdown;
		}

		if (re->EndRegistration && re->EndRegistration != hooked_EndRegistration)
		{
			orig_EndRegistration = re->EndRegistration;
			re->EndRegistration = hooked_EndRegistration;
		}

		if (re->EndFrame && re->EndFrame != hooked_EndFrame)
		{
			orig_EndFrame = re->EndFrame;
			re->EndFrame = hooked_EndFrame;
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
#ifdef USE_DEBUG
		AllocConsole();
		freopen(XorString("CONOUT$"), XorString("w"), stdout);
		printf(XorString("Loaded CCHook:Reloaded! Make sure to undefine `USE_DEBUG` for release builds.\n"));
#endif

		tools::Srand(GetTickCount() * GetCurrentProcessId());

		spoofSeed = cfg.spoofSeed;
		if (!spoofSeed)
			spoofSeed = GetTickCount() * tools::Rand();

		if (!off::Init())
		{
			printf(XorString("[WARNING] Failed to recognize 'et.exe' version. Defaulting to dynamic offsets.\n"));

			if (!off::RetrieveDynamic())
			{
				printf(XorString("[ ERROR ] Failed to retrieve dynamic offsets!\n"));
				return FALSE;
			}
		}

		printf(XorString("Waiting for game...\n"));

		// Make sure to hook even if we inject late and no DLL is loaded after us
		LdrDllNotification(0, nullptr, nullptr);

		// We dont want to spawn any threads
		void *cookie;
		typedef NTSTATUS (NTAPI *LdrRegisterDllNotification_t)(ULONG Flags, void *NotificationFunction, PVOID Context, PVOID *Cookie);
		auto LdrRegisterDllNotification = (LdrRegisterDllNotification_t)GetProcAddress(GetModuleHandleA(XorString("ntdll.dll")), XorString("LdrRegisterDllNotification"));
		LdrRegisterDllNotification(0, LdrDllNotification, nullptr, &cookie);

		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
