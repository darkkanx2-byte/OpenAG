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

// Some translation units expect a CHudESP type; alias it to our dummy type so
// a single symbol can satisfy both CHudESP and DummyHudESP references.
typedef DummyHudESP CHudESP;

extern CHudESP g_HudESP;

#endif // HUD_ESP_H
