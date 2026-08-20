#pragma once
#include "hud.h"

class CHudESP : public CHudBase
{
public:
	int Init(void) override;
	int VidInit(void) override;
	int Draw(float flTime) override;
	void Think(void) override;
	void Reset(void) override;

private:
	cvar_t* m_pCvarESP;
	cvar_t* m_pCvarESPEnemiesOnly;
	cvar_t* m_pCvarESPShowHealth;
	cvar_t* m_pCvarESPShowName;
	cvar_t* m_pCvarESPShowDistance;
	cvar_t* m_pCvarESPShowBox;
	cvar_t* m_pCvarESPShowLines;
	cvar_t* m_pCvarESPShowHead;
	cvar_t* m_pCvarESPMaxDistance;
	cvar_t* m_pCvarESPAlpha;
	cvar_t* m_pCvarESPShowWeapon;

	bool m_bKeyPressed;
	float m_flNextToggle;

	void DrawPlayerESP(cl_entity_t* player, int index);
	void Draw3DBox(vec3_t origin, int r, int g, int b, int alpha, float height);
	void DrawHealthBar(int x, int y, int health, int maxHealth);
	void DrawNameTag(int x, int y, const char* name, int r, int g, int b);
	void DrawDistance(int x, int y, float distance, int r, int g, int b);
	void DrawSnapLine(int x, int y, int r, int g, int b, int alpha);
	void DrawHeadDot(int x, int y, int r, int g, int b);
	void DrawWeaponInfo(int x, int y, int weaponId, int r, int g, int b);

	bool WorldToScreen(const vec3_t& worldPos, float& screenX, float& screenY);
	void GetTeamColor(int team, int localTeam, int& r, int& g, int& b);
	const char* GetWeaponName(int weaponId);
	bool IsValidTarget(cl_entity_t* player);
	float GetDistance(const vec3_t& a, const vec3_t& b);
};

extern CHudESP g_HudESP
;
