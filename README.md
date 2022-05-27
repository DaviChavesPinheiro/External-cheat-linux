# External Cheat - Linux
This project is a simple exemple of an external cheat on linux. Here we have two binaries: 
- game.exe (simulates a game with a variable that we want to change)
- hack.exe (is the program that will inject values direct into game.exe memory)

## Compile and Run
g++ -o game.exe Game.cpp

g++ -o hack.exe Hack.cpp

sudo ./hack.exe game.exe
