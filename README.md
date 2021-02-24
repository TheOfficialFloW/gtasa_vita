# GTA: SA Vita

<p align="center"><img src="./screenshots/game.png"></p>

This is a wrapper/port of *Grand Theft Auto: San Andreas Android* for the *PS Vita* with lots of custom patches such as:

- Fixed camera controls for flying vehicles (including the Hydra jet).
- Fixed broken facial expressions.
- Fixed cheats hash key table.
- Removed specular lighting on pedestrians.
- Added PS2-like rendering.

The port works by loading the official Android ARMv7 executable in memory, resolving its imports with native functions and patching it in order to properly run.

## Changelog

### v1.2

- Fixed memory leak in vitaGL leading to corrupted textures.
- Added ability to map L2/R2 to the front touchpad on the top. Thanks to adjutantt.

### v1.1

- Changed to Xbox 360 buttons mapping (while keeping PS3 icons).
  - Fixes fighting moves
  - Allows switching target using L2/R2.
  - Changes bunny hop from double X to L1.
- Fixed rain and classic shadows rendering.
- Improved loading speed. Thanks to Graphene.
- Fixed thread scheduling. Thanks to Graphene.
- Fixed crash on exit.
- Removed redundant mobile controls menu entries.

### v1.0

- Initial release.

## Setup Instructions (For End Users)

(If you have already installed the game and want to update to a newer release, you can simply install [GTASA.vpk](https://github.com/TheOfficialFloW/gtasa_vita/releases/download/v1.2/GTASA.vpk) on your *PS Vita*).

In order to properly install the game, you'll have to follow these steps precisely:

- Install [kubridge](https://github.com/TheOfficialFloW/kubridge/releases/) and [FdFix](https://github.com/TheOfficialFloW/FdFix/releases/) by copying `kubridge.skprx` and `fd_fix.skprx` to your taiHEN plugins folder (usually `ux0:tai`) and adding two entries to your `config.txt` under `*KERNEL`:
  
```
  *KERNEL
  ux0:tai/kubridge.skprx
  ux0:tai/fd_fix.skprx
```

**Note** Don't install fd_fix.skprx if you're using repatch plugin

- **Optional**: Install [PSVshell](https://github.com/Electry/PSVshell/releases) to overclock your device to 500Mhz.
- Install `libshacccg.suprx`, if you don't have it already, by following [this guide](https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx).
- Obtain your copy of *Grand Theft Auto: San Andreas v2.00* legally (`com.rockstargames.gtasager` is not supported!) for Android in form of an `.apk` file and one or more `.obb` files (usually `main.8.com.rockstargames.gtasa.obb` and `patch.8.com.rockstargames.gtasa.obb` located inside the `/sdcard/android/obb/com.rockstargames.gtasa/`) folder. [You can get all the required files directly from your phone](https://stackoverflow.com/questions/11012976/how-do-i-get-the-apk-of-an-installed-app-without-root-access) or by using an apk extractor you can find in the play store. The apk can be extracted with whatever Zip extractor you prefer (eg: WinZip, WinRar, etc...) since apk is basically a zip file. You can rename `.apk` to `.zip` to open them with your default zip extractor.
- Open the apk with your zip explorer, extract the `assets` folder from your `.apk` file to `ux0:data` and rename it to `gtasa`. The result would be `ux0:data/gtasa/`
- Still in the apk, extract the file `libGTASA.so` from the `lib/armeabi-v7a` folder to `ux0:data/gtasa`. 
- Open the `main.8.com.rockstargames.gtasa.obb` with your zip explorer (`.obb` files are zip files just like `.apk` files so just rename the `.obb` to `.zip`) and extract the contents to `ux0:data/gtasa`.
- Same as before, open the `patch.8.com.rockstargames.gtasa.obb` with the zip explorer and extract the contents inside the zip to `ux0:data/gtasa`.
- Download the [gamefiles.zip](https://github.com/TheOfficialFloW/gtasa_vita/releases/download/v1.2/gamefiles.zip) and extract the contents to `ux0:data/gtasa` (if it ask about overwriting files, say yes).
- **Optional**: For a more authentic console experience, copy the file `ux0:data/gtasa/data/360Default1280x720.cfg` to `ux0:data/gtasa/` and rename it from `360Default1280x720.cfg` to `Adjustable.cfg`. This file is a leftover from the Xbox 360 version and provides you the console HUD (e.g. radar on bottom left).
- Install [GTASA.vpk](https://github.com/TheOfficialFloW/gtasa_vita/releases/download/v1.2/GTASA.vpk) on your *PS Vita*.

If you have followed the steps correctly, this is how your `ux0:data/gtasa` folder should look like.

<p align="center"><img src="./screenshots/layout.png"></p>

## Configurator App

After fully installing the port, you'll be able to configure it with the Configurator app.  
The Configurator app will allow users to enable or disable a set of optimizations, patches and renderer alterations to best match users taste.  
You can launch the Configurator app by clicking on the `Configuration` button located on the LiveArea section of the port as shown in the following screenshot.

<p align="center"><img src="./screenshots/livearea_configuration.png"></p>

## Tips and Tricks

- In order to reduce occasional stutters in-game, delete both `ux0:data/gtasa/scache_small_low.txt` and `ux0:data/gtasa/scache_small.txt`, then create a copy of the `ux0:data/gtasa/scache.txt` file to have two version of it. (for example `scache(1).txt` so in the end you end up with both `scache.txt` and `scache(1).txt` inside the `ux0:data/gtasa/` folder), then rename `scache.txt` to `scache_small.txt` and `scache(1).txt` to `scache_small_low.txt` . This will however make the loading screen longer since it needs to compile more shaders ahead.
- You can input PC cheats by pressing L+SELECT to open the on-screen keyboard. See [CHEATS.md](CHEATS.md) for available and unavailable cheats (you can input cheat codes in lowercase as well as uppercase).
- The L2/R2 buttons are mapped to the rear touchpad on the top and the L3/R3 buttons are mapped to the front touchpad on the bottom. With v1.2 and higher, you can map L2/R2 to the front touchpad on the top.
- You can open the map by holding START and then releasing.
- In order to save storage on your Memory Card, you can safely delete all files in sub-folders of `ux0:data/gtasa/texdb` which end with:
  - `.dxt.dat`, `.dxt.tmb`, `dxt.toc`
  - `.etc.dat`, `.etc.tmb`, `etc.toc`
- To get local freeroam coop and rampages working you will need to replace the main scripts with those of the PS3 version:
  - Obtain `mainPS3.scm` and `scriptps3.img` from the `/PS3_GAME/USRDIR/PS3Data.obb -> /data/script` folder of a PS3 copy. In order to extract the `.obb` file rename it to `.zip` and use your favorite archive extractor.
  - Rename the files to `mainV1.scm` and `scriptv1.img` and copy them to both `ux0:data/gtasa/data/` and `ux0:data/gtasa/data/script/` replacing both original files in both directories each time.
  - Keep in mind that saves made with the old files will be incompatible with a game using the new files and vice-versa.
  - To use the multiplayer functionality navigate to a corresponding marker (a list can be found here [Marker locations](https://gta.fandom.com/wiki/Multiplayer_in_GTA_San_Andreas) ). You will have to have another controller connected via Bluetooth and set up as controller number 2 (you can configure that via the qick menu, accessible by holding the PS-Button on that controller once connected).
  - On a PSVita (as opposed to on a PSTV) you will also need to use [MiniVitaTV](https://github.com/TheOfficialFloW/MiniVitaTV) to connect the controller in the first place.
  
## Build Instructions (For Developers)

In order to build the loader, you'll need a [vitasdk](https://github.com/vitasdk) build fully compiled with softfp usage.  
You can find a precompiled version here: [Linux](https://github.com/vitasdk/buildscripts/suites/1824103476/artifacts/35161735) / [Windows](https://github.com/vitasdk/buildscripts/suites/1836262288/artifacts/35501612).  
Additionally, you'll need these libraries to be compiled as well with `-mfloat-abi=softfp` added to their CFLAGS:

- [openal-soft](https://github.com/Rinnegatamante/openal-soft)

  - Remove `-ftree-vectorize` from `Makefile.vita`.

  - ```bash
    make -f Makefile.vita install
    ```

- [libmathneon](https://github.com/Rinnegatamante/math-neon)

  - ```bash
    make install
    ```

- [vitaShaRK](https://github.com/Rinnegatamante/vitaShaRK)

  - ```bash
    make install
    ```

- [imgui-vita](https://github.com/Rinnegatamante/imgui-vita)

  - ```bash
    make install
    ```

- [kubridge](https://github.com/TheOfficialFloW/kubridge)

  - ```bash
    mkdir build && cd build
    cmake .. && make install
    ```

- [vitaGL](https://github.com/Rinnegatamante/vitaGL)

  - ````bash
    make HAVE_SBRK=1 SOFTFP_ABI=1 SHARED_RENDERTARGETS=1 NO_DEBUG=1 install
    ````

Finally, in the folder of `gtasa_vita`, install SceLibc stubs using:

```bash
make -C libc_bridge install
```

After all these requirements are met, you can compile the loader with the following commands:

```bash
mkdir build && cd build
cmake .. && make
```

## Credits

- Rinnegatamante for porting the renderer using vitaGL, providing the companion app and making various improvements to the port.
- aap for porting PS2-rendering aka skygfx.
- Freakler for providing LiveArea assets.
- frangarcj, fgsfds and Bythos for graphics-related stuff.
- CBPS/SonicMastr for PIB, which was used on earlier stages of development.
- isage for the native audio backend for OpenAL-Soft.
- JonathanERC and gtagmodding for the Cheat List 
