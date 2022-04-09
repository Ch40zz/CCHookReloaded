#pragma once

namespace off
{
	// These offsets are all only dependent on the ET.exe version, not the loaded mod.

	class COffsets
	{
	private:
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
		};

	private:
		uint32_t m_Timedatestamp;
		SOffsets m_Offsets;

		bool m_isRelative;
		uintptr_t m_imageBase;

		template <typename T>
		T MakeAddress(uintptr_t offset)
		{
			return m_isRelative ? (T)(m_imageBase + offset) : (T)(offset);
		}

	public:
		COffsets(uint32_t Timedatestamp, const SOffsets &offsets)
			: m_Timedatestamp(Timedatestamp)
			, m_Offsets(offsets)
		{
		}

		void Init()
		{
			m_imageBase = (uintptr_t)GetModuleHandleA(nullptr);

			const auto *idh = (IMAGE_DOS_HEADER*)m_imageBase;
			const auto *inh = (IMAGE_NT_HEADERS*)(m_imageBase + idh->e_lfanew);

			m_isRelative = !!(inh->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE);
		}

		uint32_t Timedatestamp() const
		{
			return m_Timedatestamp;
		}

		bool IsEtLegacy() const
		{
			return m_isRelative;
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

		// ET:Legacy 2.79.0
		{
			/*m_Timedatestamp =*/ 0x61C4B0CA,

			/*Offsets =*/
			{
				/*refExport =*/ 0x0297C6C0,				// "----- Initializing Renderer ----\n", "Couldn't initialize refresh", "cl_paused"
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
		}
	};

	// Default current offsets to 2.60b
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
				cur.Init();

				return true;
			}
		}

		return false;
	}
}
