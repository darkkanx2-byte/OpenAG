#include "hud.h"
#include "util_vector.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "entity_state.h"
#include "APIProxy.h"
#include "hud_radar.h"
#include <math.h>

CHudRadar g_HudRadar;

int CHudRadar::Init()
{
	m_pCvarRadar = CVAR_CREATE("radar_enabled", "1", FCVAR_ARCHIVE);
	m_pCvarRadarPos = CVAR_CREATE("radar_pos", "20 20", FCVAR_ARCHIVE);
	m_pCvarRadarSize = CVAR_CREATE("radar_size", "150", FCVAR_ARCHIVE);
	m_pCvarRadarRange = CVAR_CREATE("radar_range", "1500", FCVAR_ARCHIVE);
	m_pCvarRadarZoom = CVAR_CREATE("radar_zoom", "1.0", FCVAR_ARCHIVE);
	m_pCvarRadarRotate = CVAR_CREATE("radar_rotate", "1", FCVAR_ARCHIVE);
	m_pCvarRadarShowNames = CVAR_CREATE("radar_shownames", "0", FCVAR_ARCHIVE);
	m_pCvarRadarShowLines = CVAR_CREATE("radar_showlines", "1", FCVAR_ARCHIVE);
	m_pCvarRadarAlpha = CVAR_CREATE("radar_alpha", "180", FCVAR_ARCHIVE);

	m_fZoom = 1.0f; m_flNextZoomChange = 0.0f;
	m_iFlags = HUD_ACTIVE; gHUD.AddHudElem(this);
	return 0;
}

int CHudRadar::VidInit() { return 1; }
void CHudRadar::Reset() { m_fZoom = 1.0f; m_flNextZoomChange = 0.0f; }
void CHudRadar::Think() { m_fZoom = m_pCvarRadarZoom->value; if (m_fZoom < 0.1f) m_fZoom = 0.1f; if (m_fZoom > 5.0f) m_fZoom = 5.0f; }

int CHudRadar::Draw(float flTime)
{
	if (m_pCvarRadar->value == 0.0f) return 0;

	if (sscanf(m_pCvarRadarPos->string, "%d %d", &m_iRadarX, &m_iRadarY) != 2) { m_iRadarX = 20; m_iRadarY = 20; }
	m_iRadarSize = (int)m_pCvarRadarSize->value;
	if (m_iRadarSize < 50) m_iRadarSize = 50; if (m_iRadarSize > 400) m_iRadarSize = 400;

	int alpha = (int)m_pCvarRadarAlpha->value; if (alpha < 0) alpha = 0; if (alpha > 255) alpha = 255;

	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer) return 0;
	if (localPlayer->curstate.health <= 0) return 0;

	int localTeam = g_iTeamNumber; vec3_t localOrigin = localPlayer->origin; float localYaw = gHUD.m_vecAngles[YAW];

	DrawRadarBackground();
	DrawLocalPlayer();

	float maxRange = m_pCvarRadarRange->value / m_fZoom;

	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		if (i == localPlayer->index) continue;
		cl_entity_t* player = gEngfuncs.GetEntityByIndex(i);
		if (!player || !player->player) continue;
		if (player->curstate.health <= 0) continue;
		if (player->curstate.solid == SOLID_NOT) continue;
		if (player->curstate.spectator) continue;

		vec3_t diff; VectorSubtract(player->origin, localOrigin, diff);
		float distance = Length(diff);
		if (distance > maxRange) continue;

		gEngfuncs.pfnGetPlayerInfo(i, &g_PlayerInfoList[i]);
		DrawRadarPlayer(player, i);
	}

	DrawRadarRangeCircle();
	return 0;
}

void CHudRadar::DrawRadarBackground()
{
	int x = m_iRadarX, y = m_iRadarY, size = m_iRadarSize;
	int alpha = (int)m_pCvarRadarAlpha->value;

	FillRGBA(x, y, size, size, 0, 0, 0, alpha / 2);
	FillRGBA(x, y, size, 2, 100, 100, 100, 255);
	FillRGBA(x, y + size - 2, size, 2, 100, 100, 100, 255);
	FillRGBA(x, y, 2, size, 100, 100, 100, 255);
	FillRGBA(x + size - 2, y, 2, size, 100, 100, 100, 255);

	int centerX = x + size / 2; int centerY = y + size / 2;
	FillRGBA(centerX - 1, y, 2, size, 50, 50, 50, 100);
	FillRGBA(x, centerY - 1, size, 2, 50, 50, 50, 100);
	FillRGBA(centerX - 1, y + size/4, 2, size/2, 30, 30, 30, 80);
	FillRGBA(x + size/4, centerY - 1, size/2, 2, 30, 30, 30, 80);

	int rangeSize = size / 2;
	FillRGBA(centerX - rangeSize, centerY - rangeSize, rangeSize * 2, 1, 40, 40, 40, 100);
	FillRGBA(centerX - rangeSize, centerY + rangeSize, rangeSize * 2, 1, 40, 40, 40, 100);
	FillRGBA(centerX - rangeSize, centerY - rangeSize, 1, rangeSize * 2, 40, 40, 40, 100);
	FillRGBA(centerX + rangeSize, centerY - rangeSize, 1, rangeSize * 2, 40, 40, 40, 100);
}

void CHudRadar::DrawLocalPlayer()
{
	int centerX = m_iRadarX + m_iRadarSize / 2; int centerY = m_iRadarY + m_iRadarSize / 2;
	int size = 6;
	for (int row = 0; row < size; row++) {
		int width = row * 2 + 1; int startX = centerX - width / 2; int startY = centerY - size + row;
		FillRGBA(startX, startY, width, 1, 255, 255, 255, 255);
	}

	float yaw = gHUD.m_vecAngles[YAW] * (3.14159f / 180.0f);
	int endX = centerX + (int)(10 * sinf(yaw)); int endY = centerY - (int)(10 * cosf(yaw));

	int dx = abs(endX - centerX), dy = abs(endY - centerY);
	int sx = (centerX < endX) ? 1 : -1; int sy = (centerY < endY) ? 1 : -1;
	int err = dx - dy; int cx = centerX, cy = centerY;
	while (true) {
		FillRGBA(cx, cy, 1, 1, 255, 255, 255, 200);
		if (cx == endX && cy == endY) break;
		int e2 = 2 * err; if (e2 > -dy) { err -= dy; cx += sx; } if (e2 < dx) { err += dx; cy += sy; }
	}
}

void CHudRadar::DrawRadarPlayer(cl_entity_t* player, int index)
{
	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer) return;
	vec3_t localOrigin = localPlayer->origin; float localYaw = gHUD.m_vecAngles[YAW];

	float rx, ry; WorldToRadar(player->origin, localOrigin, localYaw, rx, ry);
	int screenX = m_iRadarX + (int)rx; int screenY = m_iRadarY + (int)ry;

	int r, g, b; GetTeamColor(player->curstate.team, g_iTeamNumber, r, g, b);
	bool isEnemy = (gHUD.m_Teamplay && player->curstate.team != g_iTeamNumber) || !gHUD.m_Teamplay;

	int dotSize = isEnemy ? 4 : 3;
	FillRGBA(screenX - dotSize/2, screenY - dotSize/2, dotSize, dotSize, r, g, b, 255);

	if (isEnemy) {
		FillRGBA(screenX - dotSize/2 - 1, screenY - dotSize/2 - 1, dotSize + 2, 1, r, g, b, 150);
		FillRGBA(screenX - dotSize/2 - 1, screenY + dotSize/2, dotSize + 2, 1, r, g, b, 150);
		FillRGBA(screenX - dotSize/2 - 1, screenY - dotSize/2, 1, dotSize, r, g, b, 150);
		FillRGBA(screenX + dotSize/2, screenY - dotSize/2, 1, dotSize, r, g, b, 150);
	}

	float yaw = player->angles[YAW] * (3.14159f / 180.0f);
	int endX = screenX + (int)(8 * sinf(yaw)); int endY = screenY - (int)(8 * cosf(yaw));
	int dx = abs(endX - screenX), dy = abs(endY - screenY);
	int sx = (screenX < endX) ? 1 : -1; int sy = (screenY < endY) ? 1 : -1;
	int err = dx - dy; int cx = screenX, cy = screenY;
	while (true) {
		FillRGBA(cx, cy, 1, 1, r, g, b, 200);
		if (cx == endX && cy == endY) break;
		int e2 = 2 * err; if (e2 > -dy) { err -= dy; cx += sx; } if (e2 < dx) { err += dx; cy += sy; }
	}

	if (m_pCvarRadarShowNames->value > 0 && g_PlayerInfoList[index].name) {
		gHUD.DrawHudStringCentered(screenX, screenY + 6, const_cast<char*>(g_PlayerInfoList[index].name), r, g, b);
	}

	if (m_pCvarRadarShowLines->value > 0 && isEnemy) {
		int centerX = m_iRadarX + m_iRadarSize / 2; int centerY = m_iRadarY + m_iRadarSize / 2;
		int ldx = abs(screenX - centerX), ldy = abs(screenY - centerY);
		int lsx = (centerX < screenX) ? 1 : -1; int lsy = (centerY < screenY) ? 1 : -1;
		int lerr = ldx - ldy; int lx = centerX, ly = centerY;
		while (true) {
			FillRGBA(lx, ly, 1, 1, r, g, b, 80);
			if (lx == screenX && ly == screenY) break;
			int le2 = 2 * lerr; if (le2 > -ldy) { lerr -= ldy; lx += lsx; } if (le2 < ldx) { lerr += ldx; ly += lsy; }
		}
	}
}

void CHudRadar::WorldToRadar(const vec3_t& worldPos, const vec3_t& localOrigin, float localYaw, float& radarX, float& radarY)
{
	float dx = worldPos[0] - localOrigin[0]; float dy = worldPos[1] - localOrigin[1];
	float range = m_pCvarRadarRange->value / m_fZoom; float scale = (m_iRadarSize / 2.0f) / range;

	if (m_pCvarRadarRotate->value > 0) {
		float yawRad = -localYaw * (3.14159f / 180.0f); float cosYaw = cosf(yawRad); float sinYaw = sinf(yawRad);
		float rotatedX = dx * cosYaw - dy * sinYaw; float rotatedY = dx * sinYaw + dy * cosYaw;
		dx = rotatedX; dy = rotatedY;
	}

	radarX = (m_iRadarSize / 2.0f) + (dx * scale); radarY = (m_iRadarSize / 2.0f) - (dy * scale);
	if (radarX < 2) radarX = 2; if (radarX > m_iRadarSize - 2) radarX = m_iRadarSize - 2;
	if (radarY < 2) radarY = 2; if (radarY > m_iRadarSize - 2) radarY = m_iRadarSize - 2;
}

void CHudRadar::DrawRadarRangeCircle()
{
	int centerX = m_iRadarX + m_iRadarSize / 2; int centerY = m_iRadarY + m_iRadarSize / 2;
	char rangeStr[32]; sprintf(rangeStr, "%.0f", m_pCvarRadarRange->value / m_fZoom);
	gHUD.DrawHudStringCentered(centerX, m_iRadarY + m_iRadarSize + 2, rangeStr, 150, 150, 150);
	char zoomStr[32]; sprintf(zoomStr, "%.1fx", m_fZoom);
	gHUD.DrawHudString(m_iRadarX + m_iRadarSize - 30, m_iRadarY + m_iRadarSize + 2, ScreenWidth, zoomStr, 150, 150, 150);
}

void CHudRadar::GetTeamColor(int team, int localTeam, int& r, int& g, int& b)
{
	if (!gHUD.m_Teamplay) { r = 255; g = 50; b = 50; return; }
	if (team == localTeam) { r = 50; g = 255; b = 50; } else { r = 255; g = 50; b = 50; }
}
