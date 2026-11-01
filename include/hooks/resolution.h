#pragma once

#include "pch.h"
#include <HookLib.h>
#include <hooks/shared.h>

using AdjustWindowRectFn = decltype(&AdjustWindowRect);
inline HookHolder<AdjustWindowRectFn> adjustWindowRectHook;

inline BOOL WINAPI adjustWindowRectHandler(LPRECT r, DWORD style, BOOL menu)
{
    if (r)
    {
        std::cout << std::format("[resolution] Call to windowRectHandler with ({}, {}, {}, {})", r->left, r->top, r->right, r->bottom) << std::endl;
        if (r->left == 0 && r->top == 0 && r->right == BASE_WIDTH && r->bottom == BASE_HEIGHT)
        {
            r->right = g_width;
            r->bottom = g_height;
        }
    }

    return adjustWindowRectHook.call(r, style, menu);
}

constexpr const char *GAME_WINDOW_CLASS = "AppWindow_Polar";

inline void installResolutionHooks()
{
    if (!g_gameWnd)
    {
        HWND window = nullptr;
        while ((window = FindWindowExA(nullptr, window, GAME_WINDOW_CLASS, nullptr)))
        {
            DWORD processId = 0;
            GetWindowThreadProcessId(window, &processId);

            if (processId == GetCurrentProcessId())
            {
                std::cout << "[resolution] Found game window..." << std::endl;
                g_gameWnd = window;
                break;
            }
        }
    }

    adjustWindowRectHook = HookFactory::install(lookupModule(L"user32.dll"), "AdjustWindowRect", &adjustWindowRectHandler);
}
