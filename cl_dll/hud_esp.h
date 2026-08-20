// Stub header: hud_esp.h
// Created automatically to satisfy missing include during build.
#ifndef HUD_ESP_H
#define HUD_ESP_H

// Minimal dummy HUD object expected by hud.cpp
struct DummyHudESP {
	void Init() {}
	void VidInit() {}
	void Shutdown() {}
	void Think() {}
};

extern DummyHudESP g_HudESP;

#endif // HUD_ESP_H
