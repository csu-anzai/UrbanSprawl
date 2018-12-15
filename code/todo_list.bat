@echo off

set searchFiles=*.h *.cpp

echo -----

echo TODO LIST:
findstr -s -n -i -l "TODO(" %searchFiles%

echo -----