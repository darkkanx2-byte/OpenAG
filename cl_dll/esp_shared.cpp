#include "esp_shared.h"
#include <windows.h>
#include <cstring>
#include <cmath>

void WriteESPShared(const ESPSharedData &data)
{
	const char *name = "OpenAG_ESP_SharedMemory";
	HANDLE hMap = OpenFileMappingA(FILE_MAP_WRITE, FALSE, name);
	if (!hMap) {
		hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)sizeof(ESPSharedData), name);
		if (!hMap)
			return;
	}
	void *p = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, sizeof(ESPSharedData));
	if (p) {
		memcpy(p, &data, sizeof(data));
		UnmapViewOfFile(p);
	}
	CloseHandle(hMap);
}

void WriteTestESPFrame()
{
	ESPSharedData data;
	memset(&data, 0, sizeof(data));
	data.valid = true;
	data.playerCount = 1;
	data.players[0].valid = true;
	data.players[0].screenX = 400.0f;
	data.players[0].screenY = 300.0f;
	data.players[0].distance = 1000.0f;
	data.players[0].health = 100;
	data.players[0].team = 1;
	data.players[0].isEnemy = true;
	data.players[0].isDead = false;
	strncpy_s(data.players[0].name, "TestPlayer", _TRUNCATE);
	data.players[0].origin[0] = data.players[0].origin[1] = data.players[0].origin[2] = 0.0f;

	data.radar.localOrigin[0] = data.radar.localOrigin[1] = data.radar.localOrigin[2] = 0.0f;
	data.radar.localYaw = 0.0f;
	data.radar.playerCount = 0;

	data.espEnabled = true;
	data.radarEnabled = true;
	data.enemiesOnly = false;
	data.screenWidth = GetSystemMetrics(SM_CXSCREEN);
	data.screenHeight = GetSystemMetrics(SM_CYSCREEN);

	WriteESPShared(data);
}
