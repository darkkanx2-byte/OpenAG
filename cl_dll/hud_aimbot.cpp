#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "entity_state.h"
#include "hud_aimbot.h"
#include "hud_radar.h"
#include <math.h>

CHudAimbot g_HudAimbot;

int CHudAimbot::Init()
{
	m_pCvarAimbot = CVAR_CREATE("aimbot_enabled", "0", FCVAR_ARCHIVE);
	m_pCvarAimbotFOV = CVAR_CREATE("aimbot_fov", "10", FCVAR_ARCHIVE);
	m_pCvarAimbotSmooth = CVAR_CREATE("aimbot_smooth", "5", FCVAR_ARCHIVE);
	m_pCvarAimbotKey = CVAR_CREATE("aimbot_key", "0", FCVAR_ARCHIVE);
	m_pCvarAimbotBone = CVAR_CREATE("aimbot_bone", "0", FCVAR_ARCHIVE);
	m_pCvarAimbotTeamCheck = CVAR_CREATE("aimbot_teamcheck", "1", FCVAR_ARCHIVE);
	m_pCvarTriggerbot = CVAR_CREATE("triggerbot_enabled", "0", FCVAR_ARCHIVE);
	m_pCvarTriggerbotDelay = CVAR_CREATE("triggerbot_delay", "0", FCVAR_ARCHIVE);
	m_pCvarTriggerbotKey = CVAR_CREATE("triggerbot_key", "0", FCVAR_ARCHIVE);
	m_pCvarTriggerbotHeadOnly = CVAR_CREATE("triggerbot_headonly", "0", FCVAR_ARCHIVE);
	m_pCvarNoRecoil = CVAR_CREATE("norecoil_enabled", "0", FCVAR_ARCHIVE);
	m_pCvarNoRecoilMode = CVAR_CREATE("norecoil_mode", "1", FCVAR_ARCHIVE);

	m_bAimbotKeyPressed = false; m_bTriggerbotKeyPressed = false;
	m_flTriggerDelay = 0.0f; m_flNextTrigger = 0.0f; m_iTargetIndex = -1;
	m_iFlags = HUD_ACTIVE; gHUD.AddHudElem(this);
	return 0;
}

int CHudAimbot::VidInit() { return 1; }
void CHudAimbot::Reset() { m_bAimbotKeyPressed = false; m_bTriggerbotKeyPressed = false; m_flNextTrigger = 0.0f; m_iTargetIndex = -1; }
void CHudAimbot::Think() {}

int CHudAimbot::Draw(float flTime)
{
	if (m_pCvarAimbot->value > 0 || m_pCvarTriggerbot->value > 0 || m_pCvarNoRecoil->value > 0)
	{
		int r = 255, g = 0, b = 0;
		if (m_pCvarAimbot->value > 0) { r = 255; g = 50; b = 50; }
		else if (m_pCvarTriggerbot->value > 0) { r = 255; g = 150; b = 50; }
		else { r = 50; g = 150; b = 255; }
		char status[64];
		if (m_pCvarAimbot->value > 0) sprintf(status, "[AIM]");
		else if (m_pCvarTriggerbot->value > 0) sprintf(status, "[TRG]");
		else sprintf(status, "[NRC]");
		gHUD.DrawHudString(5, ScreenHeight - 20, ScreenWidth, status, r, g, b);
	}
	return 0;
}

void CHudAimbot::CL_CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	if (!active) return;
	if (m_pCvarNoRecoil->value > 0) RunNoRecoil(cmd);
	if (m_pCvarAimbot->value > 0) RunAimbot(cmd);
	if (m_pCvarTriggerbot->value > 0) RunTriggerbot(cmd);
}

void CHudAimbot::RunAimbot(usercmd_t* cmd)
{
	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer) return;
	if (localPlayer->curstate.health <= 0) return;
	if (g_iUser1 != 0) return;

	vec3_t aimAngle; int targetIndex;
	if (!FindBestTarget(aimAngle, targetIndex)) { m_iTargetIndex = -1; return; }
	m_iTargetIndex = targetIndex;

	vec3_t viewangles; VectorCopy(cmd->viewangles, viewangles);
	float smooth = m_pCvarAimbotSmooth->value; if (smooth < 1.0f) smooth = 1.0f; if (smooth > 20.0f) smooth = 20.0f;
	SmoothAim(viewangles, aimAngle, smooth);
	ClampAngles(viewangles); NormalizeAngles(viewangles);
	VectorCopy(viewangles, cmd->viewangles); gEngfuncs.SetViewAngles((float*)viewangles);
}

bool CHudAimbot::FindBestTarget(vec3_t& outAngle, int& outIndex)
{
	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer) return false;
	vec3_t eyePos = localPlayer->origin; eyePos[2] += 28;
	vec3_t viewangles; gEngfuncs.GetViewAngles((float*)viewangles);
	float bestFOV = m_pCvarAimbotFOV->value; if (bestFOV < 0.1f) bestFOV = 0.1f; if (bestFOV > 180.0f) bestFOV = 180.0f;
	int bestIndex = -1; vec3_t bestAngle; float bestDistance = 999999.0f;

	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		if (i == localPlayer->index) continue;
		cl_entity_t* player = gEngfuncs.GetEntityByIndex(i);
		if (!player || !player->player) continue;
		if (!IsValidTarget(player)) continue;
		if (m_pCvarAimbotTeamCheck->value > 0 && gHUD.m_Teamplay && player->curstate.team == g_iTeamNumber) continue;

		vec3_t targetPos = (m_pCvarAimbotBone->value == 0) ? GetHeadPosition(player) : player->origin;
		targetPos[2] += 36;
		if (!IsVisible(eyePos, targetPos)) continue;

		vec3_t aimAngle; CalcAngle(eyePos, targetPos, aimAngle);
		float fov = GetFOV(viewangles, aimAngle);
		if (fov < bestFOV) {
			float dist = GetDistance(eyePos, targetPos);
			if (dist < bestDistance) { bestDistance = dist; bestFOV = fov; bestIndex = i; bestAngle = aimAngle; }
		}
	}

	if (bestIndex != -1) { outAngle = bestAngle; outIndex = bestIndex; return true; }
	return false;
}

void CHudAimbot::RunTriggerbot(usercmd_t* cmd)
{
	cl_entity_t* localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer) return; if (localPlayer->curstate.health <= 0) return;
	vec3_t eyePos = localPlayer->origin; eyePos[2] += 28;
	vec3_t viewangles; VectorCopy(cmd->viewangles, viewangles);
	vec3_t forward; AngleVectors(viewangles, forward, NULL, NULL);
	vec3_t endPos; VectorMA(eyePos, 8192.0f, forward, endPos);

	pmtrace_t trace; gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(eyePos, endPos, PM_STUDIO_BOX, localPlayer->index, &trace);

	if (trace.fraction < 1.0f && trace.ent > 0) {
		cl_entity_t* hitEntity = gEngfuncs.GetEntityByIndex(trace.ent);
		if (hitEntity && hitEntity->player) {
			bool isEnemy = true; if (gHUD.m_Teamplay && m_pCvarAimbotTeamCheck->value > 0) isEnemy = (hitEntity->curstate.team != g_iTeamNumber);
			bool isDead = (hitEntity->curstate.solid == SOLID_NOT) || (hitEntity->curstate.health <= 0);
			if (isEnemy && !isDead) {
				if (m_pCvarTriggerbotHeadOnly->value > 0) {
					vec3_t headPos = GetHeadPosition(hitEntity); vec3_t traceEnd = trace.endpos;
					float distToHead = GetDistance(traceEnd, headPos);
					if (distToHead > 15.0f) return;
				}
				if (gHUD.m_flTime >= m_flNextTrigger) {
					cmd->buttons |= IN_ATTACK; m_flNextTrigger = gHUD.m_flTime + m_pCvarTriggerbotDelay->value;
				}
			}
		}
	}
}

void CHudAimbot::RunNoRecoil(usercmd_t* cmd)
{
	int mode = (int)m_pCvarNoRecoilMode->value;
	if (mode == 1) {
		// Bu mod için view.cpp'de V_DropPunchAngle hook'u gerekir
		// Şu anda sadece placeholder
	}
	else if (mode == 2) {
		// Aimbot zaten viewangles'ı düzeltiyor
	}
}

bool CHudAimbot::IsValidTarget(cl_entity_t* player)
{
	if (player->curstate.health <= 0) return false;
	if (player->curstate.solid == SOLID_NOT) return false;
	if (player->curstate.spectator) return false;
	if (player->curstate.renderfx == kRenderFxDeadPlayer) return false;
	return true;
}

float CHudAimbot::GetFOV(const vec3_t& viewangles, const vec3_t& aimAngle)
{
	vec3_t delta; VectorSubtract(aimAngle, viewangles, delta); NormalizeAngles(delta);
	return sqrt(delta[PITCH] * delta[PITCH] + delta[YAW] * delta[YAW]);
}

void CHudAimbot::SmoothAim(vec3_t& current, const vec3_t& target, float smooth)
{
	vec3_t delta; VectorSubtract(target, current, delta); NormalizeAngles(delta);
	current[PITCH] += delta[PITCH] / smooth; current[YAW] += delta[YAW] / smooth; NormalizeAngles(current);
}

void CHudAimbot::CalcAngle(const vec3_t& src, const vec3_t& dst, vec3_t& angles)
{
	vec3_t delta; VectorSubtract(dst, src, delta);
	float hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles[PITCH] = atan2f(-delta[2], hyp) * (180.0f / 3.14159f);
	angles[YAW] = atan2f(delta[1], delta[0]) * (180.0f / 3.14159f);
	angles[ROLL] = 0.0f; NormalizeAngles(angles);
}

void CHudAimbot::ClampAngles(vec3_t& angles) { if (angles[PITCH] > 89.0f) angles[PITCH] = 89.0f; if (angles[PITCH] < -89.0f) angles[PITCH] = -89.0f; }
void CHudAimbot::NormalizeAngles(vec3_t& angles) { while (angles[YAW] > 180.0f) angles[YAW] -= 360.0f; while (angles[YAW] < -180.0f) angles[YAW] += 360.0f; while (angles[PITCH] > 180.0f) angles[PITCH] -= 360.0f; while (angles[PITCH] < -180.0f) angles[PITCH] += 360.0f; angles[ROLL] = 0.0f; }
vec3_t CHudAimbot::GetHeadPosition(cl_entity_t* player) { vec3_t head = player->origin; head[2] += (player->curstate.usehull == 1) ? 12 : 28; return head; }
bool CHudAimbot::IsVisible(const vec3_t& from, const vec3_t& to) { pmtrace_t trace; gEngfuncs.pEventAPI->EV_SetTraceHull(2); gEngfuncs.pEventAPI->EV_PlayerTrace(from, to, PM_STUDIO_BOX, -1, &trace); return (trace.fraction >= 1.0f); }
float CHudAimbot::GetDistance(const vec3_t& a, const vec3_t& b) { vec3_t diff; VectorSubtract(a, b, diff); return Length(diff); }
