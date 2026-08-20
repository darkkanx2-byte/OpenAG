// Stub header: hud_radar.h
// Created automatically to satisfy missing include during build.
#ifndef HUD_RADAR_H
#define HUD_RADAR_H

struct DummyHudRadar {
	void Init() {}
	void VidInit() {}
	void Shutdown() {}
	void Think() {}
};

// Some translation units may reference CHudRadar; provide an alias so symbols
// match when only stubs are available.
typedef DummyHudRadar CHudRadar;

extern CHudRadar g_HudRadar;

#endif // HUD_RADAR_H
