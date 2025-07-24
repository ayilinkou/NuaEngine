#pragma once

#ifndef SYSTEMCLASS_H
#define SYSTEMCLASS_H

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "DirectXMath.h"

#include "ImGuiManager.h"

#include "InputClass.h"

class SystemClass
{
public:
	bool Initialise();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	static void ProcessMouseMovement();
	static void ConfineCursorToWindow();

public:
	static DirectX::XMFLOAT2 m_MouseDelta;

	static HWND m_hwnd;

	static bool ms_bShouldProcessMouse;
	static POINT ms_Center;

private:
	bool Frame();
	void InitialiseWindows(int& ScreenWidth, int& ScreenHeight);
	void ShutdownWindows();

private:
	LPCWSTR m_ApplicationName = {};
	HINSTANCE m_hInstance = {};

	ImGuiManager ImGui;

};

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

static SystemClass* ApplicationHandle = nullptr;

#endif
