#include "util_vector.h"
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "entity_state.h"
#include "APIProxy.h"
#include "hud_esp.h"
#include "hud_radar.h"
#include "../common/triangleapi.h"
#ifndef YAW
#define PITCH 0
#define YAW 1
#define ROLL 2
#endif
#include <math.h>

CHudESP g_HudESP;

static const char* GetWeaponNameById(int id)
{
	switch(id)
	{
		case 1: return "Crowbar"; case 2: return "Glock"; case 3: return "Python";
		case 4: return "MP5"; case 5: return "Crossbow"; case 6: return "Shotgun";
		case 7: return "RPG"; case 8: return "Gauss"; case 9: return "Egon";
		case 10: return "Hornetgun"; case 11: return "Grenade"; case 12: return "Tripmine";
		case 13: return "Satchel"; case 14: return "Snark"; case 15: return "USP";
		case 16: return "Deagle"; case 17: return "AK47"; case 18: return "M4A1";
		case 19: return "AWP"; case 20: return "G3SG1"; case 21: return "SG550";
		case 22: return "SG552"; case 23: return "AUG"; case 24: return "Famas";
		case 25: return "Galil"; case 26: return "Scout"; case 27: return "Knife";
		case 28: return "P90"; case 29: return "TMP"; case 30: return "MAC10";
		case 31: return "UMP45"; case 32: return "M249"; case 33: return "XM1014";
		case 34: return "M3"; case 35: return "FiveSeven"; case 36: return "Elite";
		case 37: return "P228"; case 38: return "Glock18"; case 39: return "Flashbang";
		case 40: return "HE"; case 41: return "Smoke"; case 42: return "C4";
		default: return "Unknown";
	}
}

int CHudESP::Init()
{
	m_pCvarESP = CVAR_CREATE("esp_enabled", "1", FCVAR_ARCHIVE);
	m_pCvarESPEnemiesOnly = CVAR_CREATE("esp_enemies_only", "0", FCVAR_ARCHIVE);
	m_pCvarESPShowHealth = CVAR_CREATE("esp_show_health", "1", FCVAR_ARCHIVE);
	m_pCvarESPShowName = CVAR_CREATE("esp_show_name", "1", FCVAR_ARCHIVE);
	m_pCvarESPShowDistance = CVAR_CREATE("esp_show_distance", "1", FCVAR_ARCHIVE);
	m_pCvarESPShowBox = CVAR_CREATE("esp_show_box", "1", FCVAR_ARCHIVE);
	m_pCvarESPShowLines = CVAR_CREATE("esp_show_lines", "0", FCVAR_ARCHIVE);
	m_pCvarESPShowHead = CVAR_CREATE("esp_show_head", "1", FCVAR_ARCHIVE);
	m_pCvarESPMaxDistance = CVAR_CREATE("esp_max_distance", "5000", FCVAR_ARCHIVE);
	m_pCvarESPAlpha = CVAR_CREATE("esp_alpha", "200", FCVAR_ARCHIVE);
	m_pCvarESPShowWeapon = CVAR_CREATE("esp_show_weapon", "1", FCVAR_ARCHIVE);

	m_bKeyPressed = false;
	m_flNextToggle = 0.0f;
	m_iFlags = HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 0;
}

int CHudESP::VidInit() { return 1; }
void CHudESP::Reset() { m_bKeyPressed = false; m_flNextToggle = 0.0f; }
void CHudESP::Think() { m_bKeyPressed = false; }

int CHudESP::Draw(float flTime)
{
	if (m_pCvarESP->value == 0.0f) return 0;

	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer) return 0;
	if (localPlayer->curstate.health <= 0) return 0;

	int localTeam = g_iTeamNumber;
	int alpha = (int)m_pCvarESPAlpha->value;
	if (alpha < 0) alpha = 0; if (alpha > 255) alpha = 255;

	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		if (i == localPlayer->index) continue;
		cl_entity_t* player = gEngfuncs.GetEntityByIndex(i);
		if (!player || !player->player) continue;
		if (!IsValidTarget(player)) continue;

		gEngfuncs.pfnGetPlayerInfo(i, &g_PlayerInfoList[i]);
		if (!g_PlayerInfoList[i].name) continue;

		if (m_pCvarESPEnemiesOnly->value > 0)
		{
			if (gHUD.m_Teamplay && player->curstate.team == localTeam) continue;
		}

		float distance = GetDistance(localPlayer->origin, player->origin);
		if (distance > m_pCvarESPMaxDistance->value) continue;

		DrawPlayerESP(player, i);
	}
	return 0;
}

bool CHudESP::IsValidTarget(cl_entity_t* player)
{
	if (player->curstate.health <= 0) return false;
	if (player->curstate.solid == SOLID_NOT) return false;
	if (player->curstate.spectator) return false;
	if (player->curstate.renderfx == kRenderFxDeadPlayer) return false;
	return true;
}

float CHudESP::GetDistance(const vec3_t& a, const vec3_t& b)
{
	vec3_t diff; VectorSubtract(a, b, diff); return Length(diff);
}

void CHudESP::DrawPlayerESP(cl_entity_t* player, int index)
{
	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer) return;

	vec3_t origin = player->origin;
	float height = 72.0f;
	if (player->curstate.usehull == 1) height = 36.0f;

	float screenX, screenY;
	if (!WorldToScreen(origin, screenX, screenY)) return;

	vec3_t headPos = origin; headPos[2] += height;
	float headScreenX, headScreenY;
	if (!WorldToScreen(headPos, headScreenX, headScreenY)) return;

	int r, g, b; GetTeamColor(player->curstate.team, g_iTeamNumber, r, g, b);
	int alpha = (int)m_pCvarESPAlpha->value;
	float distance = GetDistance(localPlayer->origin, origin);

	float boxHeight = fabs(headScreenY - screenY);
	float boxWidth = boxHeight / 2.2f;
	int boxX = (int)(screenX - boxWidth / 2);
	int boxY = (int)headScreenY;
	int boxW = (int)boxWidth;
	int boxH = (int)boxHeight;

	if (m_pCvarESPShowBox->value > 0)
	{
		Draw3DBox(origin, r, g, b, alpha, height);
		FillRGBA(boxX - 1, boxY - 1, boxW + 2, 2, 0, 0, 0, 255);
		FillRGBA(boxX - 1, boxY + boxH, boxW + 2, 2, 0, 0, 0, 255);
		FillRGBA(boxX - 1, boxY, 2, boxH, 0, 0, 0, 255);
		FillRGBA(boxX + boxW, boxY, 2, boxH, 0, 0, 0, 255);
		FillRGBA(boxX, boxY, boxW, 1, r, g, b, alpha);
		FillRGBA(boxX, boxY + boxH - 1, boxW, 1, r, g, b, alpha);
		FillRGBA(boxX, boxY, 1, boxH, r, g, b, alpha);
		FillRGBA(boxX + boxW - 1, boxY, 1, boxH, r, g, b, alpha);
	}

	if (m_pCvarESPShowLines->value > 0)
		DrawSnapLine((int)screenX, (int)screenY, r, g, b, alpha);

	if (m_pCvarESPShowHead->value > 0)
		DrawHeadDot((int)headScreenX, (int)headScreenY, r, g, b);

	int textY = boxY - gHUD.m_iFontHeight - 4;

	if (m_pCvarESPShowName->value > 0)
	{
		DrawNameTag((int)screenX, textY, g_PlayerInfoList[index].name, 255, 255, 255);
		textY -= gHUD.m_iFontHeight + 2;
	}

	if (m_pCvarESPShowWeapon->value > 0)
	{
		const char* weaponName = GetWeaponNameById(player->curstate.weaponmodel);
		DrawNameTag((int)screenX, textY, weaponName, 200, 200, 200);
		textY -= gHUD.m_iFontHeight + 2;
	}

	if (m_pCvarESPShowHealth->value > 0)
	{
		int health = player->curstate.health;
		if (health > 100) health = 100; if (health < 0) health = 0;
		int barWidth = 4; int barHeight = boxH;
		int barX = boxX - barWidth - 3; int barY = boxY;
		FillRGBA(barX, barY, barWidth, barHeight, 0, 0, 0, 200);
		int hr, hg, hb;
		if (health > 60) { hr = 0; hg = 255; hb = 0; }
		else if (health > 30) { hr = 255; hg = 255; hb = 0; }
		else { hr = 255; hg = 0; hb = 0; }
		int fillHeight = (int)(barHeight * (health / 100.0f));
		FillRGBA(barX, barY + (barHeight - fillHeight), barWidth, fillHeight, hr, hg, hb, 255);
	}

	if (m_pCvarESPShowDistance->value > 0)
		DrawDistance((int)screenX, boxY + boxH + 2, distance, 200, 200, 200);
}

void CHudESP::Draw3DBox(vec3_t origin, int r, int g, int b, int alpha, float height)
{
	float width = 32.0f;
	Vector corners[8];
	corners[0] = origin + Vector(-width/2.0f, -width/2.0f, 0.0f);
	corners[1] = origin + Vector(width/2.0f, -width/2.0f, 0.0f);
	corners[2] = origin + Vector(width/2.0f, width/2.0f, 0.0f);
	corners[3] = origin + Vector(-width/2.0f, width/2.0f, 0.0f);
	corners[4] = origin + Vector(-width/2.0f, -width/2.0f, height);
	corners[5] = origin + Vector(width/2.0f, -width/2.0f, height);
	corners[6] = origin + Vector(width/2.0f, width/2.0f, height);
	corners[7] = origin + Vector(-width/2.0f, width/2.0f, height);

	float sx[8], sy[8]; bool visible[8];
	for (int i = 0; i < 8; i++) visible[i] = WorldToScreen(corners[i], sx[i], sy[i]);

	auto drawLine = [&](int a, int b) {
		if (visible[a] && visible[b]) {
			int x1 = (int)sx[a], y1 = (int)sy[a];
			int x2 = (int)sx[b], y2 = (int)sy[b];
			int dx = abs(x2 - x1), dy = abs(y2 - y1);
			int sx_ = (x1 < x2) ? 1 : -1;
			int sy_ = (y1 < y2) ? 1 : -1;
			int err = dx - dy;
			while (true) {
				FillRGBA(x1, y1, 1, 1, r, g, b, alpha);
				if (x1 == x2 && y1 == y2) break;
				int e2 = 2 * err;
				if (e2 > -dy) { err -= dy; x1 += sx_; }
				if (e2 < dx) { err += dx; y1 += sy_; }
			}
		}
	};

	drawLine(0,1); drawLine(1,2); drawLine(2,3); drawLine(3,0);
	drawLine(4,5); drawLine(5,6); drawLine(6,7); drawLine(7,4);
	drawLine(0,4); drawLine(1,5); drawLine(2,6); drawLine(3,7);
}

void CHudESP::DrawHealthBar(int x, int y, int health, int maxHealth)
{
	int barWidth = 50; int barHeight = 4;
	FillRGBA(x, y, barWidth, barHeight, 0, 0, 0, 200);
	float healthPct = (float)health / (float)maxHealth;
	int fillWidth = (int)(barWidth * healthPct);
	int hr, hg, hb;
	if (healthPct > 0.6f) { hr = 0; hg = 255; hb = 0; }
	else if (healthPct > 0.3f) { hr = 255; hg = 255; hb = 0; }
	else { hr = 255; hg = 0; hb = 0; }
	FillRGBA(x, y, fillWidth, barHeight, hr, hg, hb, 255);
}

void CHudESP::DrawNameTag(int x, int y, const char* name, int r, int g, int b)
{
	int width = gHUD.GetHudStringWidth(const_cast<char*>(name));
	FillRGBA(x - width/2 - 2, y - 1, width + 4, gHUD.m_iFontHeight + 2, 0, 0, 0, 150);
	gHUD.DrawHudStringCentered(x, y, const_cast<char*>(name), r, g, b);
}

void CHudESP::DrawDistance(int x, int y, float distance, int r, int g, int b)
{
	char str[32]; sprintf(str, "%.0fm", distance / 39.37f);
	gHUD.DrawHudStringCentered(x, y, str, r, g, b);
}

void CHudESP::DrawSnapLine(int x, int y, int r, int g, int b, int alpha)
{
	int centerX = ScreenWidth / 2; int centerY = ScreenHeight;
	int dx = abs(x - centerX), dy = abs(y - centerY);
	int sx = (centerX < x) ? 1 : -1;
	int sy = (centerY < y) ? 1 : -1;
	int err = dx - dy;
	int cx = centerX, cy = centerY;
	while (true) {
		FillRGBA(cx, cy, 1, 1, r, g, b, alpha);
		if (cx == x && cy == y) break;
		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; cx += sx; }
		if (e2 < dx) { err += dx; cy += sy; }
	}
}

void CHudESP::DrawHeadDot(int x, int y, int r, int g, int b)
{
	int size = 3;
	FillRGBA(x - size, y - size, size * 2 + 1, size * 2 + 1, r, g, b, 255);
	FillRGBA(x - size + 1, y - size + 1, size * 2 - 1, size * 2 - 1, 0, 0, 0, 200);
	FillRGBA(x, y, 1, 1, 255, 255, 255, 255);
}

bool CHudESP::WorldToScreen(const vec3_t& worldPos, float& screenX, float& screenY)
{
	float screen[3];
	float w[3];
	// worldPos may be Vector; copy to raw float array for the triangle API
	worldPos.CopyToArray(w);
	int result = gEngfuncs.pTriAPI->WorldToScreen(w, screen);
	if (result != 0) return false;
	screenX = XPROJECT(screen[0]); screenY = YPROJECT(screen[1]);
	return (screenX >= 0 && screenX <= ScreenWidth && screenY >= 0 && screenY <= ScreenHeight);
}

void CHudESP::GetTeamColor(int team, int localTeam, int& r, int& g, int& b)
{
	if (!gHUD.m_Teamplay) { r = 255; g = 0; b = 0; return; }
	if (team == localTeam) { r = 0; g = 255; b = 0; } else { r = 255; g = 0; b = 0; }
}
