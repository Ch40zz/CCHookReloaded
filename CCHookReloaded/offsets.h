#pragma once

#include "tools.h"

namespace off
{
	// These offsets are all only dependent on the ET.exe version, not the loaded mod.

	class CSignature
	{
	public:
		enum class EMode
		{
			None,
			AddOffset,
			ExtractPtr,
		};

		static constexpr size_t MAX_LENGTH = 64;

		EMode m_Mode;
		uintptr_t m_Data;

		size_t m_Length;
		uint8_t m_Pattern[MAX_LENGTH];
		char m_Mask[MAX_LENGTH + 1];

	public:
		CSignature(const char *pattern, const char *mask, EMode mode = EMode::None, uintptr_t data = 0)
		{
			m_Mode = mode;
			m_Data = data;

			m_Length = (std::min)(strlen(mask), MAX_LENGTH);
			memcpy(m_Pattern, pattern, m_Length);
			memcpy(m_Mask, mask, m_Length);
			m_Mask[m_Length] = '\0';
		}

		~CSignature()
		{
			__stosb(m_Pattern, 0, sizeof(m_Pattern));
			__stosb((uint8_t*)m_Mask, 0, sizeof(m_Mask));
		}

		uintptr_t Find() const
		{
			uintptr_t address = FindPattern(m_Pattern, m_Mask);
			if (address)
			{
				switch (m_Mode)
				{
				case EMode::None:		return address;
				case EMode::AddOffset:	return address + m_Data;
				case EMode::ExtractPtr:	return *(uintptr_t*)(address + m_Data);
				}
			}

			return 0;
		}

	private:
		static uintptr_t FindPattern(const uint8_t *pattern, const char *mask)
		{
			const auto imageBase = (uintptr_t)GetModuleHandleA(nullptr);
			const auto *idh = (IMAGE_DOS_HEADER*)imageBase;
			const auto *inh = (IMAGE_NT_HEADERS*)(imageBase + idh->e_lfanew);
			const auto *ish = IMAGE_FIRST_SECTION(inh);

			for (size_t i = 0; i < inh->FileHeader.NumberOfSections; i++)
			{
				if (!(ish[i].Characteristics & IMAGE_SCN_MEM_EXECUTE))
					continue;

				uint8_t *address = tools::FindPattern((uint8_t*)imageBase + ish[i].VirtualAddress, ish[i].SizeOfRawData, pattern, mask);
				if (address)
					return (uintptr_t)address;
			}

			return 0;
		}
	};

	class COffsets
	{
	public:
		struct SOffsets
		{
			uintptr_t refExport;
			uintptr_t VM_Call_vmMain;
			uintptr_t SCR_UpdateScreen;
			uintptr_t currentVM;
			uintptr_t cgvm;
			uintptr_t kbuttons;
			uintptr_t viewangles;
			uintptr_t clc_challenge;
			uintptr_t reliableCommands;
			uintptr_t clc_reliableSequence;
			uintptr_t netchan_remoteAddress;
			uintptr_t fs_searchpaths;
			uintptr_t tr_numImages;
			uintptr_t tr_images;

			bool IsValid()
			{
				// All offsets must be != 0

				constexpr size_t elementCount = sizeof(*this) / sizeof(uintptr_t);

				for (size_t i = 0; i < elementCount; i++)
					if ((&refExport)[i] == 0)
						return false;

				return true;
			}
		};

	private:
		uint32_t m_Timedatestamp;
		SOffsets m_Offsets;

		bool m_IsRelative;
		uintptr_t m_ImageBase;

		template <typename T>
		T MakeAddress(uintptr_t offset)
		{
			return m_IsRelative ? (T)(m_ImageBase + offset) : (T)(offset);
		}

	public:
		COffsets(uint32_t timedatestamp, const SOffsets &offsets)
			: m_Timedatestamp(timedatestamp)
			, m_Offsets(offsets)
		{
		}

		bool Init()
		{
			m_ImageBase = (uintptr_t)GetModuleHandleA(nullptr);

			const auto *idh = (IMAGE_DOS_HEADER*)m_ImageBase;
			const auto *inh = (IMAGE_NT_HEADERS*)(m_ImageBase + idh->e_lfanew);

			m_IsRelative = !!(inh->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE);

			return m_Offsets.IsValid();
		}

		uint32_t Timedatestamp() const
		{
			return m_Timedatestamp;
		}

		bool IsEtLegacy() const
		{
			return m_IsRelative;
		}

		uintptr_t GetRelImageBase()
		{
			return m_IsRelative ? m_ImageBase : 0;
		}

		refexport_t *refExport() { return MakeAddress<refexport_t*>(m_Offsets.refExport); }
		uintptr_t VM_Call_vmMain() { return MakeAddress<uintptr_t>(m_Offsets.VM_Call_vmMain); }
		SCR_UpdateScreen_t SCR_UpdateScreen() { return MakeAddress<SCR_UpdateScreen_t>(m_Offsets.SCR_UpdateScreen); }
		vm_t *&currentVM() { return *MakeAddress<vm_t**>(m_Offsets.currentVM); }
		vm_t *&cgvm() { return *MakeAddress<vm_t**>(m_Offsets.cgvm); }
		kbutton_t *kbuttons() { return MakeAddress<kbutton_t*>(m_Offsets.kbuttons); }
		vec_t *viewangles() { return MakeAddress<vec_t*>(m_Offsets.viewangles); }
		int &clc_challenge() { return *MakeAddress<int*>(m_Offsets.clc_challenge); }
		char *reliableCommands() { return MakeAddress<char*>(m_Offsets.reliableCommands); }
		int &clc_reliableSequence() { return *MakeAddress<int*>(m_Offsets.clc_reliableSequence); }
		netadr_t *remoteAddress() { return MakeAddress<netadr_t*>(m_Offsets.netchan_remoteAddress); }
		searchpath_t *&fs_searchpaths() { return *MakeAddress<searchpath_t**>(m_Offsets.fs_searchpaths); }
		int &tr_numImages() { return *MakeAddress<int*>(m_Offsets.tr_numImages); }
		image_t **tr_images() { return MakeAddress<image_t**>(m_Offsets.tr_images); }
	};

	inline COffsets offsets[]
	{
		// <Reserved for dynamic offsets>
		{
			/*m_Timedatestamp =*/ ~1ul, 
			/*Offsets =*/ {}
		},

		// ET 2.60b
		{
			/*m_Timedatestamp =*/ 0x445F5790,

			/*Offsets =*/
			{
				/*refExport =*/ 0x013E8BC0,				// "----- Initializing Renderer ----\n", "Couldn't initialize refresh", "cl_paused"
				/*VM_DllSyscall = 0x004488A0,*/			// "VM_Create: bad parms", 8B ? ? ? ? ? 8D ? 24 ? ? FF ? 04 83 C4 04 C3
				/*VM_Call_vmMain =*/ 0x00448AE2,		// "VM_Call( %i )\n"
				/*SCR_UpdateScreen =*/ 0x00414160,		// "cmd say "
				/*currentVM =*/ 0x010229F0,
				/*cgvm =*/ 0x0168FB14,					// "VM_Create on cgame failed"
				/*kbuttons =*/ 0x008359D8,				// "+left" => handler => push offset; call IN_KeyDown
				/*viewangles =*/ 0x013EEA88,			// "%f : %f\n"
				/*clc_challenge =*/ 0x015D6B08,			// "challenge: %d\n"
				/*reliableCommands =*/ 0x015D6B1C,		// "Client command overflow"
				/*clc_reliableSequence =*/ 0x015D6B14,
				/*netchan_remoteAddress =*/ 0x0165F7C4,	// "cl_maxpackets"
				/*cl_cmds = 0x013EE1C0,*/				// "CL_GetUserCmd: %i >= %i"
				/*cl_cmdNumber = 0x013EE8C0,*/
				/*fs_searchpaths =*/ 0x09DAA94,			// "Filesystem call made without initialization\n"
				/*tr_numImages =*/ 0x0178A684,			// "R_CreateImage: MAX_DRAWIMAGES hit\n"
				/*tr_images =*/ 0x0178A688,
			}
		},

		// RTCW MP Pro 1.4 (Not working yet, only for testing)
		{
			/*m_Timedatestamp =*/ 0x613E28A2,

			/*Offsets =*/
			{
				/*refExport =*/ 0x00FAF080,				// "----- Initializing Renderer ----\n", "Couldn't initialize refresh", "cl_paused"
				/*VM_DllSyscall = 0x0043BDC0,*/			// "VM_Create: bad parms", 8B ? ? ? ? ? 8D ? 24 ? ? FF ? 04 83 C4 04 C3
				/*VM_Call_vmMain =*/ 0x0043BA08,		// "VM_Call( %i )\n"
				/*SCR_UpdateScreen =*/ 0x00414870,		// "cmd say "
				/*currentVM =*/ 0x00AA1008,
				/*cgvm =*/ 0x000FAF108,					// "VM_Create on cgame failed"
				/*kbuttons =*/ 0x008BA818,				// "+left" => handler => push offset; call IN_KeyDown
				/*viewangles =*/ 0x010CAEB4,			// "%f : %f\n"
				/*clc_challenge =*/ 0x01030FEC,			// "challenge: %d\n"
				/*reliableCommands =*/ 0x01031000,		// "Client command overflow"
				/*clc_reliableSequence =*/ 0x01030FF8,
				/*netchan_remoteAddress =*/ 0x010B40A4,	// "cl_maxpackets"
				/*cl_cmds = 0x010CA630,*/				// "CL_GetUserCmd: %i >= %i"
				/*cl_cmdNumber = 0x010CAD30,*/
				/*fs_searchpaths =*/ 0x00A6F800,		// "Filesystem call made without initialization\n"
				/*tr_numImages =*/ 0x01230794,			// "R_CreateImage: MAX_DRAWIMAGES hit\n"
				/*tr_images =*/ 0x01230798,
			}
		},

		// ET:Legacy 2.79.0
		{
			/*m_Timedatestamp =*/ 0x61C4B0CA,

			/*Offsets =*/
			{
				/*refExport =*/ 0x0297C6C0,				// "Couldn't initialize renderer library", "cl_paused"
				/*VM_DllSyscall = 0x00026EA0,*/			// "VM_Create: bad parms", 8B ? ? ? ? ? 8D ? 24 ? ? FF ? 04 83 C4 04 C3
				/*VM_Call_vmMain =*/ 0x00026D0B,		// "VM_Call( %i )\n"
				/*SCR_UpdateScreen =*/ 0x00044570,		// "cmd say "
				/*currentVM =*/ 0x005914D8,
				/*cgvm =*/ 0x0297C78C,					// "VM_Create on cgame failed"
				/*kbuttons =*/ 0x005A04F0,				// "+left" => handler => push offset; call IN_KeyDown
				/*viewangles =*/ 0x02B91C68,			// "%f : %f\n"
				/*clc_challenge =*/ 0x02AF3D38,			// "challenge: %d\n"
				/*reliableCommands =*/ 0x02AF3D4C,		// "Client command overflow"
				/*clc_reliableSequence =*/ 0x02AF3D44,
				/*netchan_remoteAddress =*/ 0x02B7BE30,	// "cl_maxpackets"
				/*cl_cmds = 0x02B913A0,*/				// "CL_GetUserCmd: %i >= %i"
				/*cl_cmdNumber = 0x02B91AA0,*/
				/*fs_searchpaths =*/ 0x0053F2C4,		// "Filesystem call made without initialization\n"
				/*tr_numImages =*/ 0x031B050C,			// "R_CreateImage: MAX_DRAWIMAGES hit\n"
				/*tr_images =*/ 0x031B0510,
			}
		},

		// ET:Legacy 2.80.0
		{
			/*m_Timedatestamp =*/ 0x6251C920,

			/*Offsets =*/
			{
				/*refExport =*/ 0x0297C4E0,				// "Couldn't initialize renderer library", "cl_paused"
				/*VM_DllSyscall = 0x00026D80,*/			// "VM_Create: bad parms", 8B ? ? ? ? ? 8D ? 24 ? ? FF ? 04 83 C4 04 C3
				/*VM_Call_vmMain =*/ 0x00026CEB,		// "VM_Call( %i )\n"
				/*SCR_UpdateScreen =*/ 0x00044480,		// "cmd say "
				/*currentVM =*/ 0x005914F8,
				/*cgvm =*/ 0x0297C5AC,					// "VM_Create on cgame failed"
				/*kbuttons =*/ 0x0005A0510,				// "+left" => handler => push offset; call IN_KeyDown
				/*viewangles =*/ 0x02B91A88,			// "%f : %f\n"
				/*clc_challenge =*/ 0x02AF3B58,			// "challenge: %d\n"
				/*reliableCommands =*/ 0x02AF3B6C,		// "Client command overflow"
				/*clc_reliableSequence =*/ 0x02AF3B64,
				/*netchan_remoteAddress =*/ 0x02B7BC50,	// "cl_maxpackets"
				/*cl_cmds = 0x02B911C0,*/				// "CL_GetUserCmd: %i >= %i"
				/*cl_cmdNumber = 0x02B918C0,*/
				/*fs_searchpaths =*/ 0x0053F2E4,		// "Filesystem call made without initialization\n"
				/*tr_numImages =*/ 0x031B032C,			// "R_CreateImage: MAX_DRAWIMAGES hit\n"
				/*tr_images =*/ 0x031B0330,
			}
		},
	};

	// Default current offsets to dynamic offsets
	inline COffsets &cur = offsets[0];


	static inline bool Init()
	{
		const auto imageBase = (uintptr_t)GetModuleHandleA(nullptr);
		const auto *idh = (IMAGE_DOS_HEADER*)imageBase;
		const auto *inh = (IMAGE_NT_HEADERS*)(imageBase + idh->e_lfanew);

		for (const auto &offset : offsets)
		{
			// Find matching offsets based on PE header TimeDateStamp of the main executable
			if (inh->FileHeader.TimeDateStamp == offset.Timedatestamp())
			{
				cur = offset;
				if (cur.Init())
					return true;
			}
		}

		return false;
	}

	static inline bool RetrieveDynamic()
	{
		auto FindSignature = [](const CSignature *sigs, size_t count) -> uintptr_t {
			for (size_t i = 0; i < count; i++)
			{
				uintptr_t address = sigs[i].Find();
				if (address)
					return address - cur.GetRelImageBase();
			}

			return 0;
		};

		CSignature refExport[] = 
		{
			// B9 ? ? ? ? BF ? ? ? ? 68 ? ? ? ? F3 A5
			{ XorString("\xB9\x00\x00\x00\x00\xBF\x00\x00\x00\x00\x68\x00\x00\x00\x00\xF3\xA5"), XorString("x????x????x????xx"), CSignature::EMode::ExtractPtr, 6 },
		};
		CSignature VM_Call_vmMain[] = 
		{
			// FF 75 ? FF 75 ? FF ? 83 C4 44
			{ XorString("\xFF\x75\x00\xFF\x75\x00\xFF\x00\x83\xC4\x44"), XorString("xx?xx?x?xxx"), CSignature::EMode::AddOffset, 8 },
		};
		CSignature SCR_UpdateScreen[] = 
		{
			// 83 3D ? ? ? ? 0 74 ? A1 ? ? ? ? 40 83 F8 02
			{ XorString("\x83\x3D\x00\x00\x00\x00\x00\x74\x00\xA1\x00\x00\x00\x00\x40\x83\xF8\x02"), XorString("xx????xx?x????xxxx") },
		};
		CSignature currentVM[] = 
		{
			// 8B ? ? ? ? ? 85 ? 74 ? 83 ? 90 00 00 00 00 74
			{ XorString("\x8B\x00\x00\x00\x00\x00\x85\x00\x74\x00\x83\x00\x90\x00\x00\x00\x00\x74"), XorString("x?????x?x?x?xxxxxx"), CSignature::EMode::ExtractPtr, 2 },
		};
		CSignature cgvm[] = 
		{
			// 6A 07 FF 35 ? ? ? ? E8 ? ? ? ? 83 C4 14 5D C3
			{ XorString("\x6A\x07\xFF\x35\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x14\x5D\xC3"), XorString("xxxx????x????xxxxx"), CSignature::EMode::ExtractPtr, 4 },
		};
		CSignature kbuttons[] = 
		{
			// -/-
			{ XorString(""), XorString("") },
		};
		CSignature viewangles[] = 
		{
			// F3 0F 58 ? F3 0F 11 25 ? ? ? ? C3
			{ XorString("\xF3\x0F\x58\x00\xF3\x0F\x11\x25\x00\x00\x00\x00\xC3"), XorString("xxx?xxxx????x"), CSignature::EMode::ExtractPtr, 8 },
		};
		CSignature clc_challenge[] = 
		{
			// 83 ? 04 8A ? 32 35
			{ XorString("\x83\x00\x04\x8A\x00\x32\x35"), XorString("x?xx?xx"), CSignature::EMode::ExtractPtr, 7 },
		};
		CSignature reliableCommands[] = 
		{
			// 0F B6 ? C1 ? 0A 05 ? ? ? ? 50
			{ XorString("\x0F\xB6\x00\xC1\x00\x0A\x05\x00\x00\x00\x00\x50"), XorString("xx?x?xx????x"), CSignature::EMode::ExtractPtr, 7 },
		};
		CSignature clc_reliableSequence[] = 
		{
			// 8B ? ? ? ? ? 83 C4 ? 8D ? 00 FF FF FF
			{ XorString("\x8B\x00\x00\x00\x00\x00\x83\xC4\x00\x8D\x00\x00\xFF\xFF\xFF"), XorString("x?????xx?x?xxxx"), CSignature::EMode::ExtractPtr, 2 },
		};
		CSignature netchan_remoteAddress[] = 
		{
			// 66 83 3D ? ? ? ? 02 74 ? 0F
			{ XorString("\x66\x83\x3D\x00\x00\x00\x00\x02\x74\x00\x0F"), XorString("xxx????xx?x"), CSignature::EMode::ExtractPtr, 3 },
		};
		CSignature fs_searchpaths[] = 
		{
			// BB ? ? ? ? 7E ? BE ? ? ? ? 8B 0B 8B FB
			{ XorString("\xBB\x00\x00\x00\x00\x7E\x00\xBE\x00\x00\x00\x00\x8B\x0B\x8B\xFB"), XorString("x????x?x????xxxx"), CSignature::EMode::ExtractPtr, 1 },
		};
		CSignature tr_numImages[] = 
		{
			// 8B ? ? ? ? ? 81 ? 00 08 00 00 75 ? 68
			{ XorString("\x8B\x00\x00\x00\x00\x00\x81\x00\x00\x08\x00\x00\x75\x00\x68"), XorString("x?????x?xxxxx?x"), CSignature::EMode::ExtractPtr, 2 },
		};
		CSignature tr_images[] = 
		{
			// 8B 04 ? ? ? ? ? 83 ? ? ? 74
			{ XorString("\x8B\x04\x00\x00\x00\x00\x00\x83\x00\x00\x00\x74"), XorString("xx?????x???x"), CSignature::EMode::ExtractPtr, 3 },
		};


		cur = offsets[0];
		cur.Init();

		COffsets::SOffsets off;
		off.refExport = FindSignature(refExport, std::size(refExport));
		off.VM_Call_vmMain = FindSignature(VM_Call_vmMain, std::size(VM_Call_vmMain));
		off.SCR_UpdateScreen = FindSignature(SCR_UpdateScreen, std::size(SCR_UpdateScreen));
		off.currentVM = FindSignature(currentVM, std::size(currentVM));
		off.cgvm = FindSignature(cgvm, std::size(cgvm));
		off.kbuttons = FindSignature(kbuttons, std::size(kbuttons));
		off.viewangles = FindSignature(viewangles, std::size(viewangles));
		off.clc_challenge = FindSignature(clc_challenge, std::size(clc_challenge));
		off.reliableCommands = FindSignature(reliableCommands, std::size(reliableCommands));
		off.clc_reliableSequence = FindSignature(clc_reliableSequence, std::size(clc_reliableSequence));
		off.netchan_remoteAddress = FindSignature(netchan_remoteAddress, std::size(netchan_remoteAddress));
		off.fs_searchpaths = FindSignature(fs_searchpaths, std::size(fs_searchpaths));
		off.tr_numImages = FindSignature(tr_numImages, std::size(tr_numImages));
		off.tr_images = FindSignature(tr_images, std::size(tr_images));

		cur = COffsets(0, off);
		return cur.Init();
	}
}
