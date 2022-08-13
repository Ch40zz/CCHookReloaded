#pragma once

#define NOMINMAX
#define NODRAWTEXT
#define _CRT_SECURE_NO_WARNINGS

// Whether to display debug information. Undefine for release builds!
#define USE_DEBUG

// System includes
#include <WinSock2.h>
#include <windows.h>
#include <winternl.h>
#include <intrin.h>
#include <stdint.h>
#include <stdio.h>
#include <array>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <iostream>
#include <sstream>

// Include ET SDK (2.60b)
#include "etsdk.h"

// Project includes
#include "base64.h"
#include "crc32.h"
#include "sha1.h"
#include "obfuscation.h"


#ifndef USE_DEBUG
#define printf(...) do { } while(false);
#endif


// Extern functions
extern "C" intptr_t __fastcall SpoofCall12(uintptr_t spoofed_retaddr, uintptr_t target, intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, 
	intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, uintptr_t edi, uintptr_t esi, uintptr_t real_retaddr);
extern "C" intptr_t __fastcall SpoofCall12_Steam(uintptr_t spoofed_retaddr, uintptr_t target, intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5,
	intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, uintptr_t edi, uintptr_t esi, uintptr_t ebp, uintptr_t real_retaddr);
extern "C" intptr_t __fastcall SpoofCall16(uintptr_t spoofed_retaddr, uintptr_t target, intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, 
	intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, 
	intptr_t a16, uintptr_t edi, uintptr_t esi, uintptr_t real_retaddr);


// Missing Windows structs
typedef union _LDR_DLL_NOTIFICATION_DATA
{
	ULONG Flags;					//Reserved.
	PUNICODE_STRING FullDllName;	//The full path name of the DLL module.
	PUNICODE_STRING BaseDllName;	//The base file name of the DLL module.
	PVOID DllBase;					//A pointer to the base address for the DLL in memory.
	ULONG SizeOfImage;				//The size of the DLL image, in bytes.
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;


// Missing Game structs

#define MAX_RELIABLE_COMMANDS 256
#define NET_Q3MARKER "\xff\xff\xff\xff"
#define NET_STATUS_REQUEST (NET_Q3MARKER "getstatus\n")
#define NET_STATUS_RESPONSE (NET_Q3MARKER "statusResponse\n")

#define MAX_ZPATH           256
#define MAX_SEARCH_PATHS    4096
#define MAX_FILEHASH_SIZE   1024
#define MAX_DRAWIMAGES		2048

#define FS_GENERAL_REF  0x01
#define FS_UI_REF       0x02
#define FS_CGAME_REF    0x04
#define FS_QAGAME_REF   0x08

#define FS_EXCLUDE_DIR 0x1
#define FS_EXCLUDE_PK3 0x2

#define MISSILE_PRESTEP_TIME 50

typedef intptr_t (*CL_CgameSystemCalls_t)(intptr_t *args);
extern CL_CgameSystemCalls_t orig_CL_CgameSystemCalls;

typedef void (*SCR_UpdateScreen_t)();

template<class t1, class t2=intptr_t, class t3=intptr_t, class t4=intptr_t, class t5=intptr_t, class t6=intptr_t,
		 class t7=intptr_t, class t8=intptr_t, class t9=intptr_t, class t10=intptr_t, class t11=intptr_t,
		 class t12=intptr_t, class t13=intptr_t, class t14=intptr_t, class t15=intptr_t, class t16=intptr_t>
static intptr_t DoSyscall(t1 a1, t2 a2={}, t3 a3={}, t4 a4={}, t5 a5={}, t6 a6={}, 
					t7 a7={}, t8 a8={}, t9 a9={}, t10 a10={}, t11 a11={},
					t12 a12={}, t13 a13={}, t14 a14={}, t15 a15={}, t16 a16={})
{
	intptr_t args[16] = 
	{
		*(intptr_t*)&a1,
		*(intptr_t*)&a2,
		*(intptr_t*)&a3,
		*(intptr_t*)&a4,
		*(intptr_t*)&a5,
		*(intptr_t*)&a6,
		*(intptr_t*)&a7,
		*(intptr_t*)&a8,
		*(intptr_t*)&a9,
		*(intptr_t*)&a10,
		*(intptr_t*)&a11,
		*(intptr_t*)&a12,
		*(intptr_t*)&a13,
		*(intptr_t*)&a14,
		*(intptr_t*)&a15,
		*(intptr_t*)&a16,
	};

	return orig_CL_CgameSystemCalls(args);
}

typedef struct fileInPack_s
{
	char                    *name;      // name of the file
	unsigned long pos;                  // file info position in zip
	struct  fileInPack_s*   next;       // next file in the hash
} fileInPack_t;

typedef struct
{
	char pakFilename[MAX_OSPATH];               // c:\quake3\baseq3\pak0.pk3
	char pakBasename[MAX_OSPATH];               // pak0
	char pakGamename[MAX_OSPATH];               // baseq3
	void* handle;                               // handle to zip file (type: unzFile)
	int checksum;                               // regular checksum
	int pure_checksum;                          // checksum for pure
	int numfiles;                               // number of files in pk3
	int referenced;                             // referenced file flags
	int hashSize;                               // hash table size (power of 2)
	fileInPack_t*   *hashTable;                 // hash table
	fileInPack_t*   buildBuffer;                // buffer with the filenames etc.
} pack_t;

typedef struct legacy_fileInPack_s
{
	char                         *name; // name of the file
	unsigned long pos;                  // file info position in zip
	struct  legacy_fileInPack_s* next;  // next file in the hash
} legacy_fileInPack_t;

typedef struct
{
	char pakPathname[MAX_OSPATH];               // c:\\etlegacy\\etmain
	char pakFilename[MAX_OSPATH];               // c:\\etlegacy\\etmain\\pak0.pk3
	char pakBasename[MAX_OSPATH];               // pak0
	char pakGamename[MAX_OSPATH];               // etmain
	void* handle;                               // handle to zip file
	int checksum;                               // regular checksum
	int pure_checksum;                          // checksum for pure
	int numfiles;                               // number of files in pk3
	int referenced;                             // referenced file flags
	int hashSize;                               // hash table size (power of 2)
	legacy_fileInPack_t **hashTable;            // hash table
	legacy_fileInPack_t *buildBuffer;           // buffer with the filenames etc.
} legacy_pack_t;

typedef struct
{
	char path[MAX_OSPATH];              // c:\quake3
	char gamedir[MAX_OSPATH];           // baseq3
} directory_t;

typedef struct searchpath_s
{
	struct searchpath_s *next;

	pack_t      *pack;					// only one of pack / dir will be non NULL
	directory_t *dir;
} searchpath_t;

typedef struct
{
	char path[MAX_OSPATH];				//< c:\\etlegacy
	char fullpath[MAX_OSPATH];			//< c:\\etlegacy\\etmain
	char gamedir[MAX_OSPATH];			//< etmain
} legacy_directory_t;

typedef struct legacy_searchpath_s
{
	struct legacy_searchpath_s *next;

	legacy_pack_t *pack;           //< only one of pack \/ dir will be non NULL
	legacy_directory_t *dir;
} legacy_searchpath_t;

struct vm_t
{
	int programStack;
	intptr_t (*systemCall)(int *parms);

	//------------------------------------

	char name[MAX_QPATH];

	// fqpath member added 2/15/02 by T.Ray
	char fqpath[MAX_QPATH + 1];

	// for dynamic linked modules
	void *dllHandle;
	intptr_t (QDECL *entryPoint)(intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5,
		intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, 
		intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16);

	// for interpreted modules
	qboolean currentlyInterpreting;

	qboolean compiled;
	byte *codeBase;
	int codeLength;

	int *instructionPointers;
	int instructionPointersLength;

	byte *dataBase;
	int dataMask;

	int stackBottom;                // if programStack < stackBottom, error

	int numSymbols;
	struct vmSymbol_s *symbols;

	int callLevel;                  // for debug indenting
	int breakFunction;              // increment breakCount on function entry to this
	int breakCount;

	// Legacy additions:
	qboolean extract;
};

typedef struct
{
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void (*Shutdown)(qboolean destroyWindow);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void (*BeginRegistration)(glconfig_t *config);
	qhandle_t (*RegisterModel)(const char *name);
	qhandle_t (*RegisterModelAllLODs)(const char *name);
	qhandle_t (*RegisterSkin)(const char *name);
	qhandle_t (*RegisterShader)(const char *name);
	qhandle_t (*RegisterShaderNoMip)(const char *name);
	void (*RegisterFont)(const char *fontName, int pointSize, fontInfo_t *font);

	void (*LoadWorld)(const char *name);
	qboolean (*GetSkinModel)(qhandle_t skinid, const char *type, char *name);    //----(SA)	added
	qhandle_t (*GetShaderFromModel)(qhandle_t modelid, int surfnum, int withlightmap);                //----(SA)	added

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void (*SetWorldVisData)(const byte *vis);

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void (*EndRegistration)(void);

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void (*ClearScene)(void);
	void (*AddRefEntityToScene)(const refEntity_t *re);
	int (*LightForPoint)(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
	void (*AddPolyToScene)(qhandle_t hShader, int numVerts, const polyVert_t *verts);
	// Ridah
	void (*AddPolysToScene)(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);
	// done.
	void (*AddLightToScene)(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);
//----(SA)
	void (*AddCoronaToScene)(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);
	void (*SetFog)(int fogvar, int var1, int var2, float r, float g, float b, float density);
//----(SA)
	void (*RenderScene)(const refdef_t *fd);

	void (*SaveViewParms)();
	void (*RestoreViewParms)();

	void (*SetColor)(const float *rgba);    // NULL = 1,1,1,1
	void (*DrawStretchPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);      // 0 = white
	void (*DrawRotatedPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);     // NERVE - SMF
	void (*DrawStretchPicGradient)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType);
	void (*Add2dPolys)(polyVert_t* polys, int numverts, qhandle_t hShader);

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void (*DrawStretchRaw)(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
	void (*UploadCinematic)(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

	void (*BeginFrame)(stereoFrame_t stereoFrame);

	// if the pointers are not NULL, timing info will be returned
	void (*EndFrame)(int *frontEndMsec, int *backEndMsec);


	int (*MarkFragments)(int numPoints, const vec3_t *points, const vec3_t projection,
							int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer);

	void (*ProjectDecal)(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime);
	void (*ClearDecals)(void);

	int (*LerpTag)(orientation_t *tag,  const refEntity_t *refent, const char *tagName, int startIndex);
	void (*ModelBounds)(qhandle_t model, vec3_t mins, vec3_t maxs);

	void (*RemapShader)(const char *oldShader, const char *newShader, const char *offsetTime);

	void (*DrawDebugPolygon)(int color, int numpoints, float* points);

	void (*DrawDebugText)(const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude);

	qboolean (*GetEntityToken)(char *buffer, int size);

	void (*AddPolyBufferToScene)(polyBuffer_t* pPolyBuffer);

	void (*SetGlobalFog)(qboolean restore, int duration, float r, float g, float b, float depthForOpaque);

	qboolean (*inPVS)(const vec3_t p1, const vec3_t p2);

	void (*purgeCache)(void);

	//bani
	qboolean (*LoadDynamicShader)(const char *shadername, const char *shadertext);
	// fretn
	void (*RenderToTexture)(int textureid, int x, int y, int w, int h);
	//bani
	int (*GetTextureId)(const char *imagename);
	void (*Finish)(void);
} refexport_t;

typedef struct
{
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void (*Shutdown)(qboolean destroyWindow);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.

	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	//  size display elements
	void (*BeginRegistration)(glconfig_t *config);
	qhandle_t (*RegisterModel)(const char *name);
	qhandle_t (*RegisterModelAllLODs)(const char *name);
	qhandle_t (*RegisterSkin)(const char *name);
	qhandle_t (*RegisterShader)(const char *name);
	qhandle_t (*RegisterShaderNoMip)(const char *name);
	void (*RegisterFont)(const char *fontName, int pointSize, void *font, qboolean extended);

	void (*LoadWorld)(const char *name);
	qboolean (*GetSkinModel)(qhandle_t skinid, const char *type, char *name);
	qhandle_t (*GetShaderFromModel)(qhandle_t modelid, int surfnum, int withlightmap);

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void (*SetWorldVisData)(const byte *vis);

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void (*EndRegistration)(void);

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void (*ClearScene)(void);
	void (*AddRefEntityToScene)(const refEntity_t *re);
	int (*LightForPoint)(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
	void (*AddPolyToScene)(qhandle_t hShader, int numVerts, const polyVert_t *verts);
	void (*AddPolysToScene)(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);
	void (*AddLightToScene)(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);
	void (*AddCoronaToScene)(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);
	void (*SetFog)(int fogvar, int var1, int var2, float r, float g, float b, float density);
	void (*RenderScene)(const refdef_t *fd);

	void (*SetColor)(const float *rgba);        /// NULL = 1,1,1,1
	void (*DrawStretchPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);          /// 0 = white
	void (*DrawRotatedPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);         /// NERVE - SMF
	void (*DrawStretchPicGradient)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType);
	void (*Add2dPolys)(polyVert_t *polys, int numverts, qhandle_t hShader);

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void (*DrawStretchRaw)(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
	void (*UploadCinematic)(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

	void (*BeginFrame)(void);

	// if the pointers are not NULL, timing info will be returned
	void (*EndFrame)(int *frontEndMsec, int *backEndMsec);


	int (*MarkFragments)(int numPoints, const vec3_t *points, const vec3_t projection,
	                     int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer);

	void (*ProjectDecal)(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime);
	void (*ClearDecals)(void);

	int (*LerpTag)(orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex);
	void (*ModelBounds)(qhandle_t model, vec3_t mins, vec3_t maxs);

	void (*RemapShader)(const char *oldShader, const char *newShader, const char *offsetTime);

	void (*DrawDebugPolygon)(int color, int numpoints, float *points);

	void (*DrawDebugText)(const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude);

	qboolean (*GetEntityToken)(char *buffer, size_t size);

	void (*AddPolyBufferToScene)(polyBuffer_t *pPolyBuffer);

	void (*SetGlobalFog)(qboolean restore, int duration, float r, float g, float b, float depthForOpaque);

	qboolean (*inPVS)(const vec3_t p1, const vec3_t p2);

	void (*purgeCache)(void);

	qboolean (*LoadDynamicShader)(const char *shadername, const char *shadertext);

	void (*RenderToTexture)(int textureid, int x, int y, int w, int h);

	int (*GetTextureId)(const char *imagename);
	void (*Finish)(void);

	/// avi output stuff
	void (*TakeVideoFrame)(int h, int w, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg);

	void (*InitOpenGL)(void);
	int (*InitOpenGLSubSystem)(void);

#if defined(USE_REFLIGHT)
	void (*AddRefLightToScene)(const refLight_t *light);
	qhandle_t (*RegisterShaderLightAttenuation)(const char *name);
#endif

	// alternative skeletal animation system
#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	qhandle_t (*RegisterAnimation)(const char *name);
	int (*CheckSkeleton)(refSkeleton_t *skel, qhandle_t model, qhandle_t anim);
	int (*BuildSkeleton)(refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac,
	                     qboolean clearOrigin);
	int (*BlendSkeleton)(refSkeleton_t *skel, const refSkeleton_t *blend, float frac);
	int (*BoneIndex)(qhandle_t hModel, const char *boneName);
	int (*AnimNumFrames)(qhandle_t hAnim);
	int (*AnimFrameRate)(qhandle_t hAnim);
#endif

} legacy_refexport_t;

typedef struct image_s
{
	char imgName[MAX_QPATH];         // game path, including extension
	int width, height;               // source image
	int uploadWidth, uploadHeight;   // after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	unsigned int texnum;             // gl texture binding

	int frameUsed;                   // for texture usage in frame statistics

	int internalFormat;
	int TMU;                         // only needed for voodoo2

	qboolean mipmap;
	qboolean allowPicmip;
	int wrapClampMode;               // GL_CLAMP or GL_REPEAT

	int hash;           // for fast building of the backupHash

	struct image_s* next;
} image_t;

typedef struct kbutton_s
{
	int down[2];		// key nums holding it down
	unsigned downtime;	// msec timestamp
	unsigned msec;		// msec down this frame if both a down and up happened
	int active;			// current state
	int wasPressed;		// set when down, not cleared when up
} kbutton_t;

typedef enum
{
	KB_LEFT,
	KB_RIGHT,
	KB_FORWARD,
	KB_BACK,
	KB_LOOKUP,
	KB_LOOKDOWN,
	KB_MOVELEFT,
	KB_MOVERIGHT,
	KB_STRAFE,
	KB_SPEED,
	KB_UP,
	KB_DOWN,
	KB_BUTTONS0,
	KB_BUTTONS1,
	KB_BUTTONS2,
	KB_BUTTONS3,
	KB_BUTTONS4,
	KB_BUTTONS5,
	KB_BUTTONS6,
	KB_BUTTONS7,
	KB_WBUTTONS0,
	KB_WBUTTONS1,
	KB_WBUTTONS2,
	KB_WBUTTONS3,
	KB_WBUTTONS4,
	KB_WBUTTONS5,
	KB_WBUTTONS6,
	KB_WBUTTONS7,
	KB_MLOOK,
	NUM_BUTTONS
} kbuttons_t;

typedef enum
{
	NA_BOT,
	NA_BAD,                 // an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
} netadrtype_t;

typedef struct
{
	netadrtype_t type;

	byte ip[4];
	byte ipx[10];

	unsigned short port;
} netadr_t;

#pragma pack(push, 1)
/*struct etpro_command
{
	char cmd[2]; // E! or C!
	char payload_hash_b64[];

	// Separated by " " (space)

	char payload_b64[];
};*/

struct etpro_payload_C_t
{
	uint8_t random[3];

	uint32_t module_hash;
	uint32_t module_detection_flags;

	// Dynamic length
	char module_info[1]; // Base(%08lx) Sections(%hd,%ld,%ld,%ld) Char(%04hx) Stamp(%08lx)

	uint8_t checksum;
}; // Size=12+dynamic

struct etpro_payload_E_t
{
	uint8_t random[3];

	uint32_t detection_flags;
	uint32_t et_hash;
	uint32_t cgame_hash;
	char etproguid[40];

	uint8_t checksum;
}; // Size=56
#pragma pack(pop)


// Static media and data
static const vec4_t colorMenuBg     = { 0.16f, 0.20f, 0.17f, 0.80f };
static const vec4_t colorMenuBgHl   = { 0.36f, 0.40f, 0.37f, 0.80f };
static const vec4_t colorMenuBgPr   = { 0.06f, 0.10f, 0.07f, 0.90f };
static const vec4_t colorMenuBo     = { 0.50f, 0.50f, 0.50f, 0.50f };
static const vec4_t colorMenuBoHl   = { 0.60f, 0.60f, 0.60f, 0.50f };
static const vec4_t colorMenuBoPr   = { 0.40f, 0.40f, 0.40f, 0.60f };
static const vec4_t colorCheckbox   = { 0.90f, 0.90f, 0.90f, 0.90f };
static const vec4_t colorCheckboxHl = { 1.00f, 1.00f, 1.00f, 1.00f };

struct hitbox_def_t
{
	vec3_t stand_offset;
	vec3_t crouch_offset;
	vec3_t prone_offset;

	vec3_t stand_offset_moving;
	vec3_t crouch_offset_moving;
	
	vec3_t size;
};

struct hitbox_t
{
	const vec_t *offset;
	const vec_t *size;
};

static const hitbox_def_t head_hitboxes[] =
{
	// stand            crouch            prone              stand moving       crouch moving      hitbox size
	{ { 0.0, 0.0, 0.0}, { 0.0, 0.0, 0.0}, { 0.0,  0.0, 0.0}, { 0.0,  0.0, 0.0}, { 0.0,  0.0, 0.0}, { 0.0, 0.0, 0.0} },	// NOMOD
	{ { 0.0, 0.0, 4.0}, { 0.0, 0.0, 4.0}, { 0.0,  0.0, 4.0}, { 0.0,  0.0, 4.0}, { 0.0,  0.0, 4.0}, {12.0,12.0,12.0} },	// ETMAIN
	{ { 3.0, 0.0, 6.5}, { 3.0,-0.5, 6.0}, { 1.0,  0.0, 7.0}, {-5.0, -1.0, 6.5}, { 1.0,  3.0, 4.5}, {12.0,12.0,12.0} },	// ETPUB FIXED
	{ { 0.3, 0.3, 7.0}, {-0.3, 0.8, 7.0}, { 0.0,  0.3, 6.9}, { 0.0,  0.0, 6.5}, { 0.0, -0.7, 7.0}, {11.0,11.0,12.0} },	// ETPRO b_realhead 1
	{ { 0.0, 0.0, 4.0}, { 0.0, 0.0, 4.0}, { 0.0,  0.0, 4.0}, { 0.0,  0.0, 4.0}, { 0.0,  0.0, 4.0}, {12.0,12.0,12.0} },	// JAYMOD
	{ { 0.3, 0.3, 7.0}, {-0.3, 0.8, 7.0}, { 0.0,  0.3, 6.9}, { 0.0,  0.0, 6.5}, { 0.0, -0.7, 7.0}, {11.0,11.0,12.0} },	// NITMOD
	{ { 0.0, 0.0, 4.0}, { 0.0, 0.0, 4.0}, { 0.0,  0.0, 4.0}, { 0.0,  0.0, 4.0}, { 0.0,  0.0, 4.0}, {12.0,12.0,12.0} },	// NOQUARTER
	{ { 0.3, 0.3, 7.0}, {-0.3, 0.8, 7.0}, { 0.0,  0.3, 6.9}, { 0.0,  0.0, 6.5}, { 0.0, -0.7, 7.0}, {11.0,11.0,12.0} },	// SILENT
	{ { 0.3, 0.3, 7.0}, {-0.3, 0.8, 7.0}, { 0.0,  0.3, 6.9}, { 0.0,  0.0, 6.5}, { 0.0, -0.7, 7.0}, {11.0,11.0,12.0} },	// LEGACY
	{ { 0.5, 0.0, 6.5}, { 0.5, 0.0, 6.5}, { 0.5,  0.0, 6.5}, { 0.5,  0.0, 6.5}, { 0.5,  0.0, 6.5}, {12.0,12.0,12.0} },	// GENERIC REALISTIC
};
static_assert(std::size(head_hitboxes) == static_cast<int>(EMod::Unknown)+1, "Not all mods are supported by 'head_hitboxes' array");

static const uint8_t etpro_sha1_salt[] = 
{
	0x18, 0x73, 0xE1, 0x02, 0xCA, 0x8A, 0xDD, 0x08, 0xBA, 0xBC, 0x1D, 0xC6, 0xE9, 0x8C, 0xCD, 0x7B, 
	0x54, 0x2A, 0xAC, 0x95, 0xAE, 0xDE, 0xB3, 0xE2, 0xA7, 0x76, 0x94, 0x34, 0xB0, 0xEC, 0xAB, 0x08, 
	0x9A, 0xFD, 0x41, 0xAF, 0x6F, 0x4F, 0x03, 0xF5, 0x1F, 0x86, 0x95, 0x97, 0xEE, 0xBB, 0x2D, 0x03, 
	0x00, 0x93, 0xA0, 0xA3, 0x67, 0x00, 0x17, 0x4B, 0xC3, 0x56, 0x82, 0xD8, 0xE8, 0xBB, 0x40, 0xFA, 
	0x9C, 0x4E, 0x17, 0x91, 0xE5, 0xAF, 0x0D, 0x25, 0xCD, 0x25, 0x44, 0xE8, 0xA8, 0x5C, 0x3A, 0x15, 
	0x8A, 0x47, 0x7E, 0x36, 0xA6, 0x2C, 0x70, 0x9B, 0xFD, 0x1E, 0x75, 0x0B, 0xEA, 0xDC, 0x7F, 0x14, 
	0x70, 0x19, 0x09, 0x97, 0xC7, 0x7F, 0x29, 0xC4, 0xA0, 0x79, 0x46, 0x4D, 0x42, 0x52, 0xBC, 0xAE, 
	0xD7, 0x42, 0xCF, 0xC5, 0x79, 0xF5, 0x6D, 0x01, 0xCB, 0x34, 0x8D, 0x5B, 0x8C, 0x15, 0xE4, 0xCE, 
	0x71, 0x41, 0x94, 0xAC, 0x1D, 0x79, 0xBB, 0xFF, 0x32, 0x89, 0xC2, 0xFB, 0x14, 0xBD, 0xC4, 0x17, 
	0x4B, 0x1C, 0x5E, 0x1E, 0x5C, 0x19, 0x3F, 0xD3, 0xF8, 0xE3, 0x8A, 0xB5, 0x9F, 0xA2, 0xB6, 0x7C, 
	0x2E, 0x1C, 0x02, 0x3C, 0x04, 0x52, 0x33, 0x75, 0x0B, 0x7E, 0x1D, 0x5B, 0x66, 0x28, 0x31, 0xCD, 
	0x81, 0x84, 0xC0, 0xEC, 0x71, 0x93, 0x18, 0x07, 0x4F, 0x38, 0xF1, 0xA6, 0xE8, 0x62, 0xF6, 0x01, 
	0x3E, 0x28, 0x9A, 0x86, 0xE8, 0x89, 0xFB, 0x2B, 0x3F, 0x67, 0xC1, 0xDD, 0x38, 0x3E, 0xB9, 0xEC, 
	0xFB, 0xB7, 0xBE, 0x23, 0x4A, 0xE9, 0x50, 0x9C, 0x49, 0xCD, 0x6A, 0x59, 0xD5, 0x80, 0x3C, 0x81, 
	0x18, 0xE5, 0xA5, 0xFD, 0xEE, 0xA3, 0xAF, 0x58, 0x76, 0x0E, 0xBF, 0x0E, 0xC5, 0x35, 0x2E, 0xFF, 
	0x90, 0xE6, 0x9A, 0xCC, 0x64, 0x59, 0x77, 0x69, 0x31, 0x11, 0x92, 0x67, 0xDD, 0xB6, 0x8E, 0x91, 
	0x11, 0x29, 0x2C, 0x15, 0x84, 0xB2, 0x01, 0xA2, 0x6D, 0xB8, 0xCA, 0xDD, 0x02, 0xBE, 0x4F, 0x32, 
	0x82, 0x4E, 0x1E, 0x9D, 0x18, 0xB9, 0x05, 0x44, 0x5B, 0xAF, 0x1D, 0xB2, 0xBE, 0x2D, 0x09, 0xA2, 
	0xD6, 0x81, 0xCD, 0x32, 0xFD, 0x48, 0xFF, 0x42, 0x8A, 0x43, 0xBC, 0x65, 0xB3, 0x54, 0x4F, 0x94, 
	0x83, 0xA6, 0x8D, 0x49, 0xAC, 0x02, 0x39, 0x57, 0x25, 0x29, 0xD4, 0x6F, 0x5E, 0x94, 0x8B, 0xB5, 
	0x2E, 0x75, 0xDF, 0x37, 0x0A, 0x72, 0xAB, 0x81, 0x56, 0x9B, 0xBE, 0xC5, 0x1E, 0x81, 0x6E, 0x65, 
	0xDE, 0x14, 0x03, 0xDA, 0xFF, 0xA2, 0xE7, 0x9A, 0xE1, 0x27, 0x6D, 0xD2, 0x04, 0x82, 0x34, 0x7C, 
	0xDF, 0x81, 0xA4, 0x61, 0xE1, 0xF9, 0xD7, 0x7D, 0xE9, 0x11, 0x5E, 0xD6, 0x40, 0x7C, 0x49, 0x46, 
	0xD1, 0xCC, 0x2A, 0x3E, 0x5D, 0x3C, 0xF4, 0xE1, 0x78, 0x04, 0xE2, 0x9F, 0xB4, 0x31, 0xB1, 0xBF, 
	0xFF, 0x38, 0x94, 0x7D, 0x09, 0x53, 0x3D, 0x69, 0x68, 0xBB, 0xF5, 0x85, 0x0A, 0xC9, 0x4A, 0x95, 
	0xE3, 0xFB, 0x63, 0xDB, 0xFD, 0xE8, 0x7A, 0x0A, 0x87, 0x5A, 0x70, 0x89, 0xE4, 0xAB, 0xF6, 0xE8, 
	0x1D, 0x79, 0xE7, 0x7B, 0xFC, 0x2B, 0xC7, 0xD1, 0x77, 0xF8, 0x20, 0xAE, 0x85, 0x24, 0xF1, 0x54, 
	0x20, 0x53, 0x09, 0x0C, 0x3C, 0x2A, 0x07, 0xB6, 0x49, 0xE3, 0x37, 0xD1, 0x93, 0xDA, 0x0E, 0x0A, 
	0x65, 0x65, 0x08, 0x5C, 0x3E, 0x73, 0x41, 0x84, 0x43, 0x5D, 0xA0, 0xD8, 0x90, 0xA9, 0x78, 0x1E, 
	0x2A, 0xC1, 0x8E, 0x23, 0x37, 0x5B, 0x25, 0x7E, 0x78, 0x5B, 0x81, 0x0E, 0xD3, 0xD5, 0x78, 0x38, 
	0x14, 0x1C, 0xBF, 0x0B, 0x1C, 0x5A, 0x20, 0xE4, 0x1C, 0xC0, 0x35, 0x36, 0x3E, 0x01, 0x30, 0x79, 
	0x59, 0x5B, 0xBF, 0x02, 0x14, 0x0E, 0xB0, 0x49, 0x77, 0x45, 0xBD, 0x41, 0x2B, 0x79, 0xB9, 0x1A, 
	0xD2, 0xEF, 0x93, 0x30, 0x7F, 0xED, 0x87, 0x7E, 0x4D, 0xA3, 0xEE, 0xE7, 0xF7, 0x1B, 0xCA, 0xD7, 
	0x57, 0x9C, 0x7E, 0xB3, 0x1F, 0x87, 0xC4, 0xE1, 0xC7, 0x82, 0xE8, 0x4F, 0xB1, 0xE7, 0xD2, 0xD5, 
	0x09, 0x45, 0xA7, 0x19, 0xF4, 0x5F, 0x7C, 0x22, 0x26, 0xA0, 0xD8, 0x18, 0x6C, 0x81, 0xB3, 0x55, 
	0xFD, 0x9E, 0xC7, 0x27, 0x43, 0x58, 0xEB, 0x72, 0x14, 0x3E, 0xEB, 0xC5, 0x46, 0xBA, 0x09, 0x39, 
	0x28, 0x29, 0x97, 0x9D, 0x76, 0x8A, 0x18, 0x25, 0x69, 0x8C, 0x98, 0x0A, 0xBD, 0x00, 0x47, 0xD7, 
	0xBB, 0x32, 0x4E, 0x7A, 0xB2, 0xD0, 0x64, 0xF3, 0x7F, 0x01, 0x76, 0xD5, 0x40, 0xFB, 0x7E, 0xEE, 
	0xFD, 0xE1, 0xCE, 0x1A, 0xC7, 0x0F, 0xB9, 0x0D, 0x52, 0x45, 0x46, 0x47, 0x29, 0xE0, 0xAE, 0xB9, 
	0x05, 0x09, 0x40, 0xD6, 0x16, 0x36, 0x89, 0x6B, 0x1E, 0xA6, 0x5A, 0x28, 0x09, 0x2D, 0x5B, 0x74, 
	0xBD, 0xD3, 0xF4, 0x53, 0xEB, 0x67, 0xE8, 0x1B, 0xC5, 0xFD, 0x14, 0x74, 0xFC, 0xA9, 0x04, 0x5F, 
	0xE0, 0x82, 0xFD, 0xF0, 0x51, 0x06, 0xC6, 0xAA, 0x7D, 0x73, 0x30, 0x5B, 0xD1, 0x2E, 0xF5, 0x0E, 
	0x41, 0xD8, 0x06, 0x3B, 0x5E, 0xF3, 0x44, 0xA6, 0x05, 0x30, 0xF5, 0x88, 0x03, 0x2E, 0x54, 0xA2, 
	0x33, 0x89, 0xC1, 0x82, 0x73, 0xC0, 0x33, 0x5A, 0xBD, 0xD5, 0xCE, 0x99, 0x12, 0x26, 0xF2, 0x0B, 
	0x75, 0x4D, 0xDA, 0x75, 0x43, 0x18, 0xD2, 0xB1, 0x01, 0xC1, 0x9F, 0x86, 0x56, 0x7A, 0xED, 0x0E, 
	0x50, 0x50, 0xD5, 0x26, 0x6F, 0x83, 0x4F, 0x94, 0x36, 0x52, 0x18, 0xCB, 0x00, 0xAE, 0x94, 0x90, 
	0x52, 0xBB, 0x30, 0x6A, 0xD2, 0xE4, 0x96, 0x07, 0x71, 0x42, 0x92, 0xC6, 0x67, 0x85, 0x30, 0xF7, 
	0xAB, 0xCA, 0x4F, 0xD8, 0x77, 0xAE, 0xA7, 0x2B, 0xBD, 0x9F, 0x2E, 0x18, 0x2B, 0x05, 0xF2, 0x30, 
	0x5B, 0xA5, 0xDE, 0x99, 0x33, 0xA5, 0x06, 0x48, 0x76, 0x2C, 0xCA, 0x4F, 0x0E, 0xA6, 0x05, 0xBD, 
	0x01, 0x0B, 0x3A, 0xE2, 0xEA, 0x94, 0xD3, 0x75, 0x7F, 0x87, 0xBE, 0x49, 0xD6, 0x52, 0x9A, 0x07, 
	0x9D, 0x19, 0xB5, 0x24, 0x99, 0xA9, 0x97, 0x1A, 0xC4, 0x2C, 0xAA, 0x16, 0x2E, 0xE4, 0x7A, 0x35, 
	0x96, 0xF7, 0x09, 0xB6, 0xFC, 0x31, 0xEA, 0xE4, 0x5E, 0x17, 0x66, 0x79, 0x84, 0x5C, 0x00, 0x0D, 
	0x98, 0x7B, 0x1C, 0x14, 0xC2, 0x98, 0xCC, 0xC3, 0x4F, 0xAB, 0x0F, 0xD9, 0x5E, 0x57, 0x4D, 0x50, 
	0x19, 0x37, 0x35, 0x59, 0x46, 0x1D, 0x38, 0xBF, 0x66, 0x35, 0x3C, 0xC1, 0x0E, 0x45, 0x84, 0xAC, 
	0x1B, 0xA9, 0x91, 0x21, 0x42, 0x7F, 0xC8, 0xFC, 0xAF, 0xC2, 0x0C, 0x73, 0x56, 0xFD, 0xFB, 0x20, 
	0xD8, 0xE7, 0x58, 0x6F, 0x7A, 0x6D, 0xEA, 0xDB, 0x1C, 0xC4, 0xC1, 0x2C, 0x26, 0x26, 0x13, 0xDF, 
	0xC3, 0xE0, 0x5E, 0x5F, 0xCC, 0x6F, 0xE4, 0x2F, 0xDD, 0x09, 0x5E, 0x20, 0x87, 0xC5, 0x74, 0x68, 
	0x21, 0xD4, 0x5C, 0x3E, 0xB6, 0xF7, 0x8E, 0x6D, 0x1D, 0x56, 0x6F, 0x60, 0xA6, 0xDA, 0x57, 0xDD, 
	0x05, 0x69, 0x43, 0xC1, 0x77, 0xDB, 0xB1, 0x34, 0x1C, 0xF8, 0x3A, 0x2D, 0xE8, 0x25, 0x28, 0x95, 
	0x3B, 0xFE, 0x58, 0xD5, 0xC6, 0x27, 0x20, 0x12, 0x43, 0x80, 0xC9, 0x2C, 0xAB, 0x0A, 0xF3, 0xA0, 
	0xFB, 0xF0, 0xC7, 0x06, 0x4D, 0x65, 0x64, 0xF9, 0x91, 0x74, 0xC7, 0x88, 0xF0, 0xAE, 0x03, 0x05, 
	0x25, 0xDD, 0x76, 0xF7, 0xAA, 0x0B, 0x1B, 0x36, 0x24, 0x1C, 0xD1, 0x9D, 0xEC, 0x88, 0x45, 0xC3, 
	0xB5, 0xBC, 0x04, 0x6A, 0x29, 0xA4, 0x3F, 0xAC, 0x43, 0xA7, 0x15, 0x73, 0xFF, 0x86, 0x53, 0xFA, 
	0x72, 0x21, 0xD7, 0xB9, 0xB9, 0x06, 0x4D, 0xDA, 0x9C, 0x0D, 0x1D, 0xE9, 0x55, 0x9F, 0x0F, 0xE5
};
