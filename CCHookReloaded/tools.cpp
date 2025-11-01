#include "pch.h"
#include "globals.h"
#include "tools.h"

#include "config.h"
#include "offsets.h"

namespace tools
{
	static unsigned int s_Seed = 0;
	int Rand()
	{
		return (((s_Seed = s_Seed * 214013L + 2531011L) >> 16) & 0x7fff);
	}
	void Srand(unsigned int seed)
	{
		s_Seed = seed;
	}

	uint8_t *FindPattern(uint8_t *start_pos, size_t search_len, const uint8_t *pattern, const char *mask)
	{
		for(uint8_t *region_it = start_pos; region_it < (start_pos + search_len); ++region_it)
		{
			if(*region_it != *pattern)
				continue;

			const uint8_t *pattern_it = pattern;
			const uint8_t *memory_it = region_it;
			const char *mask_it = mask;

			bool found = true;

			for(; *mask_it && (memory_it < (start_pos + search_len)); ++mask_it, ++pattern_it, ++memory_it)
			{
				if(*mask_it != 'x')
					continue;

				if(*memory_it != *pattern_it)
				{
					found = false;
					break;
				}
			}

			if(found)
				return region_it;
		}

		return nullptr;
	}
    void *DetourFunction(BYTE *src, const BYTE *dst, size_t len)
    {
            BYTE *jmp = (BYTE*)VirtualAlloc(0, len+5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (!jmp)
				return nullptr;

            DWORD dwback;
            VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwback);

            memcpy(jmp, src, len);
            jmp += len;
            jmp[0] = 0xE9;
            *(uint32_t*)(jmp+1) = (uint32_t)(src+len - jmp) - 5;

            src[0] = 0xE9;
            *(uint32_t*)(src+1) = (uint32_t)(dst - src) - 5;

            VirtualProtect(src, len, dwback, &dwback);

            return jmp - len;
    }
	std::filesystem::path GetModulePath(HMODULE mod)
	{
		wchar_t exePath[MAX_PATH + 1];
		GetModuleFileNameW(mod, exePath, std::size(exePath) - 1);
		return exePath;
	}
	EMod GetETMod(char *_modName)
	{
		HMODULE cgame = GetModuleHandleA(XorString("cgame_mp_x86.dll"));
		if (!cgame)
			return EMod::None;

		const auto modPath = GetModulePath(cgame).parent_path();
		const auto modName = modPath.filename().string();

		if (_modName)
			strcpy(_modName, modName.c_str());

		if (!_stricmp(modName.c_str(), XorString("etmain")))
			return EMod::EtMain;
		if (!_stricmp(modName.c_str(), XorString("etpub")))
			return EMod::EtPub;
		if (!_stricmp(modName.c_str(), XorString("etpro")))
			return EMod::EtPro;
		if (!_stricmp(modName.c_str(), XorString("jaymod")))
			return EMod::JayMod;
		if (!_stricmp(modName.c_str(), XorString("nitmod")))
			return EMod::Nitmod;
		if (!_stricmp(modName.c_str(), XorString("nq")))
			return EMod::NoQuarter;
		if (!_stricmp(modName.c_str(), XorString("silent")))
			return EMod::Silent;
		if (!_stricmp(modName.c_str(), XorString("legacy")))
			return EMod::Legacy;

		return EMod::Unknown;
	}
	bool UnlockPak()
	{
		// We simply override the checksum and pure_checksum of our
		// custom .pk3 file with the ones from pak0.pk3.
		// This will allow us to use any content of the file
		// since the server allows that exact hash.

		if (off::cur.IsEtLegacy())
		{
			legacy_pack_t* pak0 = nullptr;

			legacy_searchpath_t *search = (legacy_searchpath_t*)off::cur.fs_searchpaths();
			for (; search; search = search->next)
			{
				legacy_pack_t *pak = search->pack;
				if (!pak)
					continue;

				if (!pak0 && !_stricmp(pak->pakBasename, XorString("pak0")))
				{
					pak0 = pak;
				}
				else if (pak0 && !_stricmp(pak->pakBasename, cfg.pakName))
				{
					// Do not send the hash to server (already sent once by original pak0.pk3).
					// This ensures the server can't easily detect us simply by checking if checksum exists twice.
					pak->referenced = 0;

					// But make sure we can access all data locally because hash is whitelisted :)
					pak->checksum = pak0->checksum;
					pak->pure_checksum = pak0->pure_checksum;

					return true;
				}
			}
		}
		else
		{
			pack_t* pak0 = nullptr;

			searchpath_t *search = off::cur.fs_searchpaths();
			for (; search; search = search->next)
			{
				pack_t *pak = search->pack;
				if (!pak)
					continue;

				if (!pak0 && !_stricmp(pak->pakBasename, XorString("pak0")))
				{
					pak0 = pak;
				}
				else if (pak0 && !_stricmp(pak->pakBasename, cfg.pakName))
				{
					// Do not send the hash to server (already sent once by original pak0.pk3).
					// This ensures the server can't easily detect us simply by checking if checksum exists twice.
					pak->referenced = 0;

					// But make sure we can access all data locally because hash is whitelisted :)
					pak->checksum = pak0->checksum;
					pak->pure_checksum = pak0->pure_checksum;

					return true;
				}
			}
		}

		printf(XorString("WARNING: Failed to find '%s.pk3' - make sure you placed it in etmain folder!\n"), cfg.pakName);

		return false;
	}
	char *Info_ValueForKeyInbuff(char *s, const char *key)
	{
		bool isKey = false;

		size_t keyIndex = 0;
		size_t keySize = strlen(key);

		for(; *s; s++)
		{
			if (*s == '\\')
			{
				if (isKey)
				{
					if (keyIndex == keySize)
						return s + 1;

					keyIndex = 0;
				}

				isKey = !isKey;
				continue;
			}

			if (keyIndex >= keySize)
				keyIndex = size_t(-1);

			if (isKey && keyIndex != -1)
			{
				if (*s != key[keyIndex++])
					keyIndex = size_t(-1);
			}
		}

		return nullptr;
	}
	bool GatherSpectatorInfo(std::vector<std::string> &spectatorNames)
	{
		// Do not query data when we are not actually playing ourselves
		if (cg_snapshot.serverTime == 0 ||
			cg_snapshot.ps.pm_type != PM_NORMAL ||
			cg_snapshot.ps.pm_flags & PMF_FOLLOW)
			return false;

		// Get current network destination and fix it up for localhost if needed
		uint16_t port;
		ADDRESS_FAMILY family;
		size_t addressSize;
		uint8_t address[32];

		if (off::cur.IsEtLegacy())
		{
			const netadr_legacy_t* remoteAddress = (netadr_legacy_t*)off::cur.remoteAddress();
			switch (remoteAddress->type)
			{
			case netadrtype_legacy_t::NA_IP:
				family = AF_INET;
				port = remoteAddress->port;
				addressSize = sizeof(remoteAddress->ip);
				memcpy(address, remoteAddress->ip, addressSize);
				break;
			case netadrtype_legacy_t::NA_IP6:
				family = AF_INET6;
				port = remoteAddress->port;
				addressSize = sizeof(remoteAddress->ip6);
				memcpy(address, remoteAddress->ip6, addressSize);
				break;
			default:
				family = AF_INET;
				vmCvar_t net_port; // localhost port
				DoSyscall(CG_CVAR_REGISTER, &net_port, XorString("net_port"), XorString("27960"), 0);
				port = htons(net_port.integer);
				addressSize = sizeof(remoteAddress->ip);
				*(uint32_t*)address = 0x0100007F;
				break;
			}
		}
		else
		{
			const netadr_t* remoteAddress = off::cur.remoteAddress();
			switch (remoteAddress->type)
			{
			case netadrtype_t::NA_IP:
				family = AF_INET;
				port = remoteAddress->port;
				addressSize = sizeof(remoteAddress->ip);
				memcpy(address, remoteAddress->ip, addressSize);
				break;
			case netadrtype_t::NA_IPX:
				family = AF_IPX;
				port = remoteAddress->port;
				addressSize = sizeof(remoteAddress->ipx);
				memcpy(address, remoteAddress->ipx, addressSize);
				break;
			default:
				family = AF_INET;
				vmCvar_t net_port; // localhost port
				DoSyscall(CG_CVAR_REGISTER, &net_port, XorString("net_port"), XorString("27960"), 0);
				port = htons(net_port.integer);
				addressSize = sizeof(remoteAddress->ip);
				*(uint32_t*)address = 0x0100007F;
				break;
			}
		}

		static SOCKET sock = INVALID_SOCKET;

		// Send server status request every 500 ms
		static uint32_t lastQueryTime = 0;
		if (GetTickCount() - lastQueryTime >= 500)
		{
			// Re-Create socket if needed

			if (sock != INVALID_SOCKET)
				closesocket(sock);

			sock = socket(family, SOCK_DGRAM, IPPROTO_UDP);
			if (sock != INVALID_SOCKET)
			{
				// Make socket non-blocking for obvious reasons
				u_long mode = 1;
				ioctlsocket(sock, FIONBIO, &mode);
			}

			sockaddr_in sockAddr;
			sockAddr.sin_family = family;
			sockAddr.sin_port = port;
			memcpy(&sockAddr.sin_addr, address, addressSize);

			// UDP messages always arrive (if they arrive) in the exact same size as sent
			sendto(sock, XorString(NET_STATUS_REQUEST), sizeof(NET_STATUS_REQUEST)-1, 0, (sockaddr*)&sockAddr, sizeof(sockAddr));

			lastQueryTime = GetTickCount();
		}


		// Keep receiving data at all time

		sockaddr_in sockAddr;
		int fromlen = sizeof(sockAddr);

		// UDP messages always arrive (if they arrive) in the exact same size as sent
		char buffer[4096];
		int read = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&sockAddr, &fromlen);
		if (read <= 0)
			return false;
	
		buffer[read] = '\0';

		if (sockAddr.sin_family != family || sockAddr.sin_port != port || memcmp(&sockAddr.sin_addr, address, addressSize))
			return false;

		if (memcmp(buffer, XorString(NET_STATUS_RESPONSE), sizeof(NET_STATUS_RESPONSE) - 1))
			return false;

		// Parse the server info to find all spectators.
		// A spectator will always inherit the XP numbers of the spectated player.
		// Because XP numbers are rather unique, this can detect spectators.
		// To be 100% certain, we should filter out existing players.

		spectatorNames.clear();

		std::stringstream playersstream(buffer + sizeof(NET_STATUS_RESPONSE)-1);

		std::string playerStats;
		if (std::getline(playersstream, playerStats, '\n'))
		while (std::getline(playersstream, playerStats, '\n'))
		{
			std::stringstream statsstream(playerStats);

			// '<XP> <PING> "<NA ME>"'
			std::string _xp, _ping, name;
			std::getline(statsstream, _xp, ' ');
			std::getline(statsstream, _ping, ' ');
			std::getline(statsstream, name, '\n');

			int xp = atoi(_xp.c_str());
			int ping = atoi(_ping.c_str());

			// Get rid of quotes in the name
			if (name.front() == '"' && name.back() == '"')
			{
				name.erase(0, 1);
				name.erase(name.size() - 1);
			}

			// Ignore people who are still connecting
			if(ping == 999)
				continue;
			if (!xp || xp != cg_snapshot.ps.persistant[PERS_SCORE])
				continue;

			// We found a player who is having the same XP numbers as we do.
			// Make sure the player is not in the player list (must be spectator).

			bool isSpectator = false;
			bool playerExists = false;

			for (size_t i = 0; i < MAX_CLIENTS; i++)
			{
				auto &player = cgs_clientinfo[i];

				if (!player.valid)
					continue;
				if (strcmp(player.name, name.c_str()))
					continue;

				playerExists = true;

				if (player.teamNum == TEAM_SPECTATOR)
				{
					isSpectator = true;
					break;
				}
			}

			if (isSpectator || !playerExists)
				spectatorNames.push_back(name);
		}

		return true;
	}
	void RandomizeHexString(char *string, size_t length, char wildcard, uint32_t seed)
	{
		for (size_t i = 0; i < length; i++)
		{
			if (string[i] != wildcard)
				continue;

			const uint16_t randVal = (((seed = seed * 214013L + 2531011L) >> 16) & 0x7fff);

			const char newChar = (randVal & 0x4000)
				? 'A' + (char)(randVal % 6)
				: '0' + (char)(randVal % 10);

			string[i] = newChar;
		}
	}
}
