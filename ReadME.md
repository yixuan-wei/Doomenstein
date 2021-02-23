# Doomenstein
Author: Yixuan Wei

## Overview
This is a 2.5D remake version of *Wolfenstein*, supporting simple multiplayer mode (tested with 6 people connected in total).

Maps could be customized in XML format, players could shoot monsters or each other, and teleport to other places or maps through portals.

## Controls
1. WASD for player movement
2. Mouse for looking direction
3. Mouse left button click for shooting
4. Networking
    1. Open console: press ”~”
	2.	Open server: 
		- type in “StartMultiplayerServer”, press “enter” to submit command; 
		- default port set to 48000, if want to customize, type in command like “StartMultiplayerServer port=48000”
	3.	Connect to Server: 
		- type in “ConnectToMultiplayerServer”
		- Default server ip set to 127.0.0.1, default port set to 48000; if want to customize, type in command like “ConnectToMultiplayerServer ip=127.0.0.1 port=48000”
5.	Quit: type in “quit” in console, or press ESC when in play interface
