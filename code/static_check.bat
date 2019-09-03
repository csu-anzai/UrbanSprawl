@echo off

set searchFiles=*.h *.cpp

echo -----

echo Globals and Persists Found:
findstr -s -n -i -l "local_persist" %searchFiles%
findstr -s -n -i -l "global_var" %searchFiles%

echo -----