#pragma once

namespace tools
{
	int Rand();
	void Srand(unsigned int seed);

	uint8_t *FindPattern(uint8_t *start_pos, size_t search_len, const uint8_t *pattern, const char *mask);
	EMod GetETMod(char *_modName);
	bool UnlockPak();
	char *Info_ValueForKeyInbuff(char *s, const char *key);
	bool GatherSpectatorInfo(std::vector<std::string> &spectatorNames);
	void RandomizeHexString(char *string, size_t length, char wildcard, uint32_t seed);
}
