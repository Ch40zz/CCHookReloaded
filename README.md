# CCHook:Reloaded - A modern, mod independent open source cheat for Enemy Territory

[![CCHookReloaded](https://i.imgur.com/OBNczRr.png)](https://www.youtube.com/watch?v=JvmvVpG1D1Y "CCHook:Reloaded")

## General
This cheat is the result of a previous Proof-of-Concept made to showcase an alternative way to hook the Quake VM system in an undetected way.
The rest of the cheat then followed the same rules to stay undetected, without compromising the ease of use of engine functions.
This is the only cheat I know of that does not patch **any** read-only memory or leaves any other nasty detections such as spawned threads or incorrectly implemented hooks.


## Usage
Modify `config.h` to your liking. Many of the config values cannot be changed at runtime and are statically compiled into the binary.
Copy `cch.pk3` into your `etmain` folder and make sure to rename it. Set the new name in the config (`pakName`) without the `.pk3` extension.
Ensure that `USE_DEBUG` macro is not defined in `pch.h` if you don't want to debug the cheat. This will significantly lower detection risk.
Compile the project (Visual Studio 2022, Windows 10 SDK). It will yield 2 binaries in the ``Release`` output folder:
- `CCHookReloaded.dll`: actual cheat DLL which has to be injected into the game
- `CCHookReloaded.exe`: LdrLoadDll injector to start the game and inject the DLL

**Please make sure to rename both files before launching the injector to avoid anti-cheat detections!**
  
When the game has been started and the cheat has been injected with the injector, join any server.
The cheat will be loaded upon mod initialization. To enable/disable cheat features, you can now open the WIP ingame menu by pressing the `DEL` key on your keyboard:
![Ingame Menu](https://i.imgur.com/TyjYIr1.png)


## Hooking
The cheat utilizes the `LdrRegisterDllNotification()` API to gain code execution on every DLL load/unload without the need to create any threads.
Because every mod will load their own new copy of `cgame_mp_x86.dll` we will always have the perfect timing to place our hooks right after the mod loaded.
From there, we hook the exported render routines of the `refexport_t` struct.
This hook can be removed after a few frames if required but currently stays in tact to detect key presses in the main menu when no mod is loaded yet.
The renderer functions that get hooked are `refexport_t::Shutdown()`, `refexport_t::EndRegistration()` and `refexport_t::EndFrame()`.
Now that we are in the correct thread, we can finally hook the function we will use for rendering and logic: `vmMain()`.
This is done by finding the pointer to the `cgvm` and swapping out its `entryPoint` address with our hook:

```cpp
vm_t *_cgvm = off::cur.cgvm();
if (_cgvm)
{
	vmMain_t vmMain = _cgvm->entryPoint;
	if (vmMain && vmMain != hooked_vmMain)
	{
		orig_vmMain = vmMain;
		_cgvm->entryPoint = hooked_vmMain;
	}
}
```

This alone would be relatively easy to detect for an anti-cheat.
Thats why we have another trick up our sleeves: All anti-cheats in this game are implemented in the mod's `cgame_mp_x86.dll`.
This means that they can only execute code, when their `vmMain` gets invoked by the engine.
To mask our `vmMain` hook, we simply spoof the return address and temporarily remove all our hooks until the mod is done running and passes execution back to our cheat.
To also easily hook the syscalls made by the mod back into the engine, we have yet another trick: we locate the `currentVM` pointer, duplicate the whole VM object and replace its `systemCall` address with our own.
This hook however, needs to stay when we call into the original `vmMain`. To make it undetected, we simply swap the `currentVM` pointer with our copy.
This way an anticheat will think it has the correct `systemCall` address, while the engine **internally** will still end up calling our hook:

```cpp
intptr_t __cdecl hooked_vmMain(intptr_t id, intptr_t a1, intptr_t a2, ...)
{
	// Hook syscall func by replacing currentVM
	vm_t *curVM = off::cur.currentVM();
	memcpy(&hookVM, (void*)curVM, sizeof(hookVM));
	orig_CL_CgameSystemCalls = hookVM.systemCall;
	hookVM.systemCall = hooked_CL_CgameSystemCalls;
	off::cur.currentVM() = &hookVM;

	auto VmMainCall = [&](intptr_t _id, intptr_t _a1=0, intptr_t _a2=0, ...) -> intptr_t {
		// Restore vmMain original temporarily
		vm_t *_cgvm = off::cur.cgvm();
		_cgvm->entryPoint = orig_vmMain;

		// Spoof the return address when calling "orig_vmMain" to make sure ACs don't easily detect our hook (e.g. as ETPro does).
		// NOTE: This is not supported for ET:Legacy but support can be added if required.
		intptr_t result = SpoofCall12(off::cur.VM_Call_vmMain(), (uintptr_t)orig_vmMain, _id, _a1, _a2, ...);

		// Rehook vmMain
		_cgvm->entryPoint = hooked_vmMain;


		// Restore currentVM
		off::cur.currentVM() = curVM;

		return result;
	};
	
	intptr_t result = VmMainCall(id, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
	
	...
	
	return result;
}
```

We now have access to every `vmMain` call (engine -> mod) and every `systemCall` (mod -> engine), giving us full control over all in- and output of the mod.


## Hardware/GUID Spoofing

The game itself has no real knowledge about the underlying hardware IDs.
The only way to track a player without additional code is to look at their `etkey` that is sent to the server on connect.
These keys can be manually generated by anyone at any time and therefore do not uniquely identify a player.
To implement player tracking, mods resort to reading the Hardware-ID of the system.
This is done by querying disk drive serials or the MAC address(es) of the network cards.
To implement a hookless Hardware-ID/GUID spoofing, the cheat directly accesses the sent packet queue `clc.reliableCommands`.
This requires reverse engineering of the specific mods and is generally inferior to placing simple hooks.
If you wish to generically bypass any HWID tracking, you should therefore resort to simple hooking of the APIs in question.


## Screenshot (PBSS) cleaning

The game is driven by the OpenGL renderer.
Screenshots are therefore also taken with OpenGL.
To read the current displayed pixels, one can call `opengl32!glReadPixels()`.
Luckily, this function is pretty easy to hook by swapping a .data pointer on Windows.
OpenGL on Windows is allocating a big API dispatch table and storing the pointer to that table via TLS (thread local storage).
The cheat extracts this table address by disassembling the instructions at `glReadPixels`.
After receiving the dispatch table address, we can easily swap out any function with our hook.
The hook then proceeds to set a global boolean value, indicating that all rendering should be disabled.
To update the screen data, the engine function `SCR_UpdateScreen()` is executed.
After the screen has been cleared, the original `glReadPixels` can be invoked to gather the cleaned screen data.
This whole process usually takes no more than a few milliseconds.


## Gathering entity data

Most cheats rely on reversing each individual mod to find the address of the `cg_entities` array.
This is however not needed. Entity data is transmitted to the client each server tick and is received by the mod using the `CG_GETSNAPSHOT` syscall.
Since we can intercept all syscalls done by the mod, we can just wait for the mod to request the data and then copy it off.
This makes it even possible to modify the data the mod will receive, allowing us to modify or even delete events sent from the server.


## Unlocking unpure PAK files

Other cheats implemented this feature by hooking `FS_PureServerSetLoadedPaks()`.
This is however not really needed. One can simply walk all loaded PAKs in `fs_searchpaths`.
Find the PAK file you want to unlock. To please the server, spoof the `checksum` and `pure_checksum` fields with the values from any legit pak file such as pak0.
Now that we use the same hash as pak0, we do not want to send the hash multiple times to the server (since it was already sent).
To do this, we simply set the pak's `referenced` count to 0.
Now the client will be able to use the unpure pak file without the server even knowing about its existence.


## Possible improvements

The biggest improvement right now would be creating weapon and event lists for each supported mod and fall back to the default ET implementation only if the mod is unknown.
As it currently stands, many mods have additional and/or removed weapons, which leads to some code not working correctly on some mods.
This however is pretty trivial and might be done soon.  
Another simple improvement would be the injector. The injector is currently using `LdrLoadDll()` to inject the dll.
It is therefore easily visible in the Ldr module list. This code should probably be swapped out with a manual mapping implementation.
However, right now no mod detects the cheat even using `LdrLoadDll()`.  
Another already mentioned improvement would be adding a more generic HWID spoofing by directly hooking the Windows APIs in question.
This hasn't be done yet because the goal of this cheat was to not modify any read-only data.  
A more obvious missing feature is menu to view, enable/disable or customize certain features dynamically ingame.
I decided against adding a menu because it adds quite a lot of code that was unnecessary for showing the main hooking techniques.  
Lastly, the aimbot prediction is really just rudimentary and should be replaced with some more advanced techniques.
One big improvement would be the use of backtracking as it is done nowadays for quake (source) based such as Counter-Strike.

