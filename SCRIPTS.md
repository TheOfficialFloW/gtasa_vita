# GTA: SA Vita - Patching The Scripts

This patcher fixes the following scripts:
- Gym treadmill (doesn't register input)
- Inside Track Betting (doesn't register input)
However, some scripts are still broken:
- Swimming speed (unconfirmed)
- You tell us

## Patching the scripts

- Copy `mainV1.scm` and `scriptv1.img` from `ux0:data/gtasa/data/script/` or the `assets/data/script/` folder of your `.apk` file and put them in an empty folder on your computer.
- Download and install [Python](https://www.python.org/downloads/). Make sure to tick `Add Python 3.9 to PATH` during installation.
- Download [patcher.py](https://raw.githubusercontent.com/TheOfficialFloW/gtasa_vita/master/scripts/patcher.py) (rightclick -> Save as) and move it to your script files.
- Hold Shift and right click your directory, and choose CMD or Powershell. Then use the command `python patcher.py` in the oppened console.
- Transfer the patched script files to `ux0:data/gtasa/data/script/`, overwrite if asked.
- You may now delete your working directory and unistall Python if you wish.