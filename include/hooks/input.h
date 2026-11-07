#pragma once

#include "pch.h"
#include <HookLib.h>
#include <windowsx.h>
#include <hooks/shared.h>
#include <scanning/scanner.h>

using PeekMessageAFn = decltype(&PeekMessageA);
inline HookHolder<PeekMessageAFn> peekMessageAHook;

using GetCursorPosFn = decltype(&GetCursorPos);
inline HookHolder<GetCursorPosFn> getCursorPosHook;

using SetCursorPosFn = decltype(&SetCursorPos);
inline HookHolder<SetCursorPosFn> setCursorPosHook;

inline bool IsClientMouseMsg(UINT m)
{
    // Client-coordinate mouse messages (excludes WM_MOUSEWHEEL/HWHEEL, which use screen coords)
    return (m >= WM_MOUSEMOVE && m <= WM_MBUTTONDBLCLK) ||
           (m >= WM_XBUTTONDOWN && m <= WM_XBUTTONDBLCLK);
}

inline BOOL CALLBACK NudgeChild(HWND child, LPARAM lp)
{
    HWND parent = reinterpret_cast<HWND>(lp);
    if (GetParent(child) != parent)
        return TRUE;

    RECT client, cr;
    GetClientRect(parent, &client);
    GetWindowRect(child, &cr);

    char cls[64]{};
    GetClassNameA(child, cls, sizeof(cls));
    std::cout << std::format("[resolution] child {} is {}x{}, client {}x{}", cls, cr.right - cr.left, cr.bottom - cr.top, client.right, client.bottom) << std::endl;

    // Force a real size change so WebDriver gets WM_SIZE after it is set up
    MoveWindow(child, 0, 0, client.right - 1, client.bottom - 1, FALSE);
    MoveWindow(child, 0, 0, client.right, client.bottom, TRUE);
    return TRUE;
}

inline BOOL WINAPI peekMessageAHandler(LPMSG msg, HWND hWnd, UINT fMin, UINT fMax, UINT remove)
{
    if (!g_childFixed && g_gameWnd && IsWindowVisible(g_gameWnd))
    {
        if (!g_firstSeen)
            g_firstSeen = GetTickCount();
        else if (GetTickCount() - g_firstSeen > 500)
        {
            EnumChildWindows(g_gameWnd, NudgeChild, reinterpret_cast<LPARAM>(g_gameWnd));
            g_childFixed = true;
        }
    }

    BOOL ok = peekMessageAHook.call(msg, hWnd, fMin, fMax, remove);
    if (ok && msg && msg->hwnd && IsClientMouseMsg(msg->message))
    {
        HWND root = GetAncestor(msg->hwnd, GA_ROOT);
        if (root == g_gameWnd || (!g_gameWnd && root))
        {
            g_gameWnd = root;
            int x = GET_X_LPARAM(msg->lParam);
            int y = GET_Y_LPARAM(msg->lParam);

            msg->lParam = MAKELPARAM(scaleToBaseX(x), scaleToBaseY(y));
        }
    }

    return ok;
}

inline BOOL WINAPI getCursorPosHandler(LPPOINT pt)
{
    BOOL ok = getCursorPosHook.call(pt);
    if (ok && pt && GameHasFocus())
    {
        POINT c = *pt;
        ScreenToClient(g_gameWnd, &c);
        c = {scaleToBaseX(c.x), scaleToBaseY(c.y)};
        ClientToScreen(g_gameWnd, &c);
        *pt = c;
    }
    return ok;
}

inline BOOL WINAPI setCursorPosHandler(int x, int y)
{
    if (GameHasFocus())
    {
        POINT p{x, y};
        ScreenToClient(g_gameWnd, &p);
        p = {scaleFromBaseX(p.x), scaleFromBaseY(p.y)};
        ClientToScreen(g_gameWnd, &p);
        x = p.x;
        y = p.y;
    }

    return setCursorPosHook.call(x, y);
}

// Polar Golfer's mouse event handler (0x00422739) passes the 640x480 event coordinates straight to the cursor sprite,
// but the sprite is positioned in actual pixels lol
constexpr MemorySignature GOLF_CURSOR_SET_POSITION_SIGNATURE = {
    "\x53\x56\xD9\x44\x24\x10\x57\x8B\xF9\x8B\xB7\x80\x00\x00\x00\x8B\x1E\xE8\x00\x00\x00\x00\xD9\x44\x24\x10\x50\xE8\x00\x00\x00\x00\x50\x56\xFF\x93\xE0\x00\x00\x00",
    "xxxxxxxxxxxxxxxxxx????xxxxxx????xxxxxxxx",
    40,
};

using SpriteSetPositionFn = void(__fastcall *)(void *sprite, void *edx, float x, float y);
inline HookHolder<SpriteSetPositionFn> golfCursorSetPositionHook;

inline void __fastcall golfCursorSetPositionHandler(void *sprite, void *edx, float x, float y)
{
    golfCursorSetPositionHook.call(sprite, edx, static_cast<float>(x * scaleX()), static_cast<float>(y * scaleY()));
}

inline void installInputHooks()
{
    peekMessageAHook = HookFactory::install(lookupModule(L"user32.dll"), "PeekMessageA", &peekMessageAHandler);
    getCursorPosHook = HookFactory::install(lookupModule(L"user32.dll"), "GetCursorPos", &getCursorPosHandler);
    setCursorPosHook = HookFactory::install(lookupModule(L"user32.dll"), "SetCursorPos", &setCursorPosHandler);

    void *cursorSetPosition = CSigScan(nullptr).Scan(GOLF_CURSOR_SET_POSITION_SIGNATURE);
    if (cursorSetPosition)
    {
        std::cout << "[input] Polar Golfer cursorSetPosition found at " << cursorSetPosition << std::endl;
        golfCursorSetPositionHook = HookFactory::install(cursorSetPosition, &golfCursorSetPositionHandler);
    }
}