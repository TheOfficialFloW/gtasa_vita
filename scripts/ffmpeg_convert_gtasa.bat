cd %~dp0
mkdir "output\AA"
mkdir "output\ADVERTS"
mkdir "output\AMBIENCE"
mkdir "output\BEATS"
mkdir "output\CH"
mkdir "output\CO"
mkdir "output\CR"
mkdir "output\CUTSCENE"
mkdir "output\DS"
mkdir "output\HC"
mkdir "output\MH"
mkdir "output\MR"
mkdir "output\NJ"
mkdir "output\RE"
mkdir "output\RG"
mkdir "output\TK"
FOR %%A IN (input\AA\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\AA\%%~nA.mp3"
FOR %%A IN (input\ADVERTS\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\ADVERTS\%%~nA.mp3"
FOR %%A IN (input\AMBIENCE\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -b:a 128k "output\AMBIENCE\%%~nA.mp3"
FOR %%A IN (input\BEATS\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\BEATS\%%~nA.mp3"
FOR %%A IN (input\CH\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\CH\%%~nA.mp3"
FOR %%A IN (input\CO\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\CO\%%~nA.mp3"
FOR %%A IN (input\CR\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\CR\%%~nA.mp3"
FOR %%A IN (input\CUTSCENE\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\CUTSCENE\%%~nA.mp3"
FOR %%A IN (input\DS\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\DS\%%~nA.mp3"
FOR %%A IN (input\HC\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\HC\%%~nA.mp3"
FOR %%A IN (input\MH\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\MH\%%~nA.mp3"
FOR %%A IN (input\MR\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\MR\%%~nA.mp3"
FOR %%A IN (input\NJ\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\NJ\%%~nA.mp3"
FOR %%A IN (input\RE\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\RE\%%~nA.mp3"
FOR %%A IN (input\RG\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\RG\%%~nA.mp3"
FOR %%A IN (input\TK\*.ogg) DO ffmpeg -i "%%A" -af "asplit[a],ametadata=select:key=lavfi.aphasemeter.phase:value=-0.005:function=less,pan=1c|c0=c0,aresample=async=1:first_pts=0,[a]amix,volume=0dB" -codec:a libmp3lame -ac 1 -q:a 2 "output\TK\%%~nA.mp3"
pause
