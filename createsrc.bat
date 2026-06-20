@echo off
echo %date%_%time%
set "my_hours=%time: =0%"
set "my_hours=%my_hours:~0,2%"
set "my_minutes=%time:~3,2%"
echo %my_hours%-%my_minutes%

pause

REM Merge the files
copy /a CMakeLists.txt + main.cpp + mainwindow.h + mainwindow.cpp + setlistmanager.h + setlistmanager.cpp ChordLabSource_%my_hours%-%my_minutes%_%date%.txt
echo Done...
pause
