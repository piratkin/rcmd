@echo off
echo Cleaning ...

rmdir Debug /s /q
rmdir Release /s /q
rmdir xCmdSvc\Debug /s /q
rmdir xCmdSvc\Release /s /q

del *.ncb /s
del *.opt /s
del *.plg /s
del *.aps /s
del *.clw /s
del *.positions /s
