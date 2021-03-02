# GTA: SA Vita - Restoring The Cut Songs

### Table of Contents
+ [GTA: SA Vita - Restoring The Cut Songs](#gta-sa-vita---restoring-the-cut-songs)
  + [Removed Songs](#removed-songs)
  + [Prerequisites](#prerequisites)
  + [Restoring The Cut Songs](#restoring-the-cut-songs)
+ [Credits](#credits)

## Removed Songs

<table><thead><tr><th>No.</th><th>Station</th><th>Song</th></tr></thead><tbody><tr><td>1</td><td>Playback FM</td><td>Ultramagnetic MC’s - Critical Beatdown</td></tr><tr><td>2</td><td rowspan="3">Radio Los Santos</td><td>2 Pac - I Don’t Give A f*ck</td></tr><tr><td>3</td><td>Compton’s Most Wanted - Hood Took Me Under</td></tr><tr><td>4</td><td>NWA - Express Yourself</td></tr><tr><td>5</td><td rowspan="3">Bounce FM</td><td>Fatback - Yum Yum</td></tr><tr><td>6</td><td>Roy Ayers - Running Away</td></tr><tr><td>7</td><td>The Gap Band - You Dropped A Bomb On Me</td></tr><tr><td>8</td><td rowspan="2">KDST</td><td>Tom Petty - Running Down A Dream</td></tr><tr><td>9</td><td>Joe Cocker - Woman to Woman</td></tr><tr><td>10</td><td rowspan="2">K-JAH West Radio</td><td>Black Harmony - Don’t Let It Go to Your Head</td></tr><tr><td>11</td><td>Blood Sisters - Ring My Bell</td></tr><tr><td>12</td><td rowspan="2">Radio:X</td><td>Ozzy Osbourne - Hellraiser</td></tr><tr><td>13</td><td>Rage Against the Machine - Killing in the Name</td></tr><tr><td>14</td><td rowspan="6">Master Sounds 98.3</td><td>Charles Wright - Express Yourself</td></tr><tr><td>15</td><td>The Blackbyrds - Rock Creek Park</td></tr><tr><td>16</td><td>James Brown - Funky President</td></tr><tr><td>17</td><td>Macer &amp; The Macks</td></tr><tr><td>18</td><td>James Brown - The Payback</td></tr><tr><td>19</td><td>The JB’s - Grunt</td></tr></tbody></table>

## Getting Started

- Firstly, you have to get the missing files. Use a disk version that has the cut songs, or downgrade your Steam/Rockstar Launcher copy. You can find how to do that online.
- Download [SAAT 1.10 by alcy](https://web.archive.org/web/20070305050639/http://pdescobar.home.comcast.net:80/gta/saat/SAAT_release_1_10.zip) and extract it.
- Download [SAAF by nick7 build 239](https://gta.nick7.com/programs/saaf/saaf_build_239.zip) and extract it.

## Restoring The Cut Songs

- Once you have an older version of the game, extract the `STREAMS` audio files with *SAAT 1.10 by alcy* in the root of the `saat` folder.
- Use the command prompt on the directory you extracted the files to. Press **Win**+**R** and type `cmd <your_directory>`, for example `cmd D:/saat/`. Then use the command `saat_stream.exe -e <stream_file(s)> <target_directory>`, for example `saat_stream.exe -e BEATS newfolder` in the oppened console.
- It will export them as `track_001.ogg`, `track_002.ogg` etc, which you'll have to convert to `.mp3` format using audio editing/converter software, for example Sound Forge 9.0.
- Convert them with 56kbps and 22050hz mono highest quality settings (you can try higher - max would be 128kbps if you are willing to test it) from `.ogg` to `.mp3` and make sure you uncheck the id3 settings so the tracks won't have any id3 tags on them.
  - Format Factory works too with a sample rate of 22050, a bit rate of 128kbps, audio channel set to 1 and VBR to 0, also set output to source file folder.
- After the conversion, compress the converted `.mp3` files into `.zip` files for each folder meaning `CH`, `DS`, `RJ`, etc, and compression set to "store" with 7z or WinRAR.
- Rename the file extension from `.zip` to `.osw` for each one of them but leave their original name untouched. For example: `DS.osw`, `CH.osw`, etc.
- Use *SAAF by nick7* to open each `.osw` file, it'll create an `.osw.idx` file.
- Finally, After creating the `.osw.idx` files for each one of them, just copy all of the `.osw` with their `.osw.idx` files to the `STREAMS` folder on your vita in `ux0:data/gtasa/audio/STREAMS` and overwrite if asked.
- You should be able to enjoy the game with all the songs on the latest build.

## Credits
