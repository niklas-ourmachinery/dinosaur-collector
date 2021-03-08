@echo off

robocopy publish dino-dino /mir

cd dino-dino\plugins

del PhysXCommon_64.dll
del PhysXCooking_64.dll
del PhysXDevice64.dll
del PhysXFoundation_64.dll
del PhysXGpu_64.dll
del PhysX_64.dll
del tm_physx.dll

del assimp-vc141-mt.dll
del tm_assimp.dll

del discord_game_sdk.dll
del tm_collaboration_discord.dll

del tm_crunch.dll

cd ..

rem dino-dino.exe

cd ..