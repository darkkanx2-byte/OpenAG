#pragma once
#include "hud.h"

class CHudRadar : public CHudBase
{
public:
	int Init(void) override;
	int VidInit(void) override;
	int Draw(float flTime) override;
	void Think(void) override;
	void Reset(void) override;

private:
	cvar_t* m_pCvarRadar;
	cvar_t* m_pCvarRadarPos;
	cvar_t* m_pCvarRadarSize;
	cvar_t* m_pCvarRadarRange;
	cvar_t* m_pCvarRadarZoom;
	cvar_t* m_pCvarRadarRotate;
	cvar_t* m_pCvarRadarShowNames;
	cvar_t* m_pCvarRadarShowLines;
	cvar_t* m_pCvarRadarAlpha;

	int m_iRadarX;
	int m_iRadarY;
	int m_iRadarSize;
	float m_fZoom;
	float m_flNextZoomChange;

	void DrawRadarBackground();
	void DrawLocalPlayer();
	void DrawRadarPlayer(cl_entity_t* player, int index);
	void DrawRadarRangeCircle();
	void WorldToRadar(const vec3_t& worldPos, const vec3_t& localOrigin, float localYaw, float& radarX, float& radarY);
	void GetTeamColor(int team, int localTeam, int& r, int& g, int& b);
};

extern CHudRadar g_HudRadar;
