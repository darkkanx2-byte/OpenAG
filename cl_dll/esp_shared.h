#pragma once

#include <cstddef>

struct SharedPlayerData
{
	bool valid; float screenX, screenY; float distance; int health; int team; bool isEnemy; bool isDead; char name[32]; float origin[3];
};

struct SharedRadarData
{
	float relativeX, relativeY; float yaw; int team; bool isEnemy; char name[32];
};

struct ESPSharedData
{
	bool valid; int playerCount; SharedPlayerData players[32];
	struct { float localOrigin[3]; float localYaw; int playerCount; SharedRadarData radarPlayers[32]; } radar;
	bool espEnabled; bool radarEnabled; bool enemiesOnly; int screenWidth; int screenHeight;
};

// Implementations are in esp_shared.cpp to avoid pulling Windows headers into many
// translation units and colliding with engine typedefs (HSPRITE, etc.).
void WriteESPShared(const ESPSharedData &data);
void WriteTestESPFrame();
