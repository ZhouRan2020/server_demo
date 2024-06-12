@echo off

SET CURRENT_PATH=%cd%
SET PROTOC_PATH=%CURRENT_PATH%

SET PATH=%PATH%;%PROTOC_PATH%

set target_path=.
echo 编译: texus_room.proto cpp && "%PROTOC_PATH%\protoc.exe" --cpp_out="%target_path%/" --proto_path "./" texus_room.proto 
echo 编译: texus_room.proto csharp && "%PROTOC_PATH%\protoc.exe" --csharp_out="%target_path%/" --proto_path "./" texus_room.proto 

echo 操作完成，按任意键退出
echo 完成时间 %date:~0,10% %time:~0,8%

pause>nul&exit