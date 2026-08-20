#pragma once
#include "hud.h"

class CHudAimbot : public CHudBase
{
public:
	int Init(void) override;
	int VidInit(void) override;
	int Draw(float flTime) override;
	void Think(void) override;
	void Reset(void) override;
	void CL_CreateMove(float frametime, struct usercmd_s *cmd, int active);

private:
	cvar_t* m_pCvarAimbot; cvar_t* m_pCvarAimbotFOV; cvar_t* m_pCvarAimbotSmooth;
	cvar_t* m_pCvarAimbotKey; cvar_t* m_pCvarAimbotBone; cvar_t* m_pCvarAimbotTeamCheck;
	cvar_t* m_pCvarTriggerbot; cvar_t* m_pCvarTriggerbotDelay; cvar_t* m_pCvarTriggerbotKey;
	cvar_t* m_pCvarTriggerbotHeadOnly; cvar_t* m_pCvarNoRecoil; cvar_t* m_pCvarNoRecoilMode;

	bool m_bAimbotKeyPressed; bool m_bTriggerbotKeyPressed; float m_flTriggerDelay; float m_flNextTrigger;
	int m_iTargetIndex; vec3_t m_vecTargetAngle;

	void RunAimbot(usercmd_t* cmd); void RunTriggerbot(usercmd_t* cmd); void RunNoRecoil(usercmd_t* cmd);
	bool FindBestTarget(vec3_t& outAngle, int& outIndex); bool IsValidTarget(cl_entity_t* player);
	float GetFOV(const vec3_t& viewangles, const vec3_t& aimAngle); void SmoothAim(vec3_t& current, const vec3_t& target, float smooth);
	void CalcAngle(const vec3_t& src, const vec3_t& dst, vec3_t& angles); void ClampAngles(vec3_t& angles); void NormalizeAngles(vec3_t& angles);
	vec3_t GetTargetBone(cl_entity_t* player, int bone); vec3_t GetHeadPosition(cl_entity_t* player);
	bool IsVisible(const vec3_t& from, const vec3_t& to);
};

extern CHudAimbot g_HudAimbot;
