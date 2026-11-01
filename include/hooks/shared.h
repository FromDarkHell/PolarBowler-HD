#pragma once

#include <pch.h>
#include <cmath>

constexpr int BASE_WIDTH = 640;
constexpr int BASE_HEIGHT = 480;

inline int g_width = 960;
inline int g_height = 720;

inline HWND g_gameWnd = nullptr;

inline double scaleX()
{
    return g_width / static_cast<double>(BASE_WIDTH);
}

inline double scaleY()
{
    return g_height / static_cast<double>(BASE_HEIGHT);
}

inline long scaleFromBaseX(long value)
{
    return lround(value * scaleX());
}

inline long scaleFromBaseY(long value)
{
    return lround(value * scaleY());
}

inline long scaleToBaseX(long value)
{
    return lround(value / scaleX());
}

inline long scaleToBaseY(long value)
{
    return lround(value / scaleY());
}

inline bool GameHasFocus()
{
    return g_gameWnd && GetForegroundWindow() == g_gameWnd;
}

inline DWORD g_firstSeen = 0;
inline bool g_childFixed = false;
