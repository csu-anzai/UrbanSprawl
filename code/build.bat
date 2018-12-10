@echo off

REM -MTd for debug build
set commonFlagsCompiler=-MTd -nologo -Gm- -GR- -fp:fast -EHa -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -FC -Z7 -DURBAN_INTERNAL=1 -DURBAN_SLOW=1 -DURBAN_WIN32=1
set commonFlagsLinker= -incremental:no -opt:ref -subsystem:console ../libs/SDL2.lib ../libs/SDL2main.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 64-bit
del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp

cl %commonFlagsCompiler% ..\code\urban.cpp -LD /link -incremental:no -opt:ref -PDB:urban_%random%.pdb -EXPORT:Game_GetAudioSamples -EXPORT:Game_UpdateRender
del lock.tmp
cl %commonFlagsCompiler% ..\code\sdl_urban.cpp /link %commonFlagsLinker%
popd