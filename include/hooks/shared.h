#pragma once

#include <HookLib.h>
#include <pch.h>
#include <scanning/scanner.h>
#include <cmath>
#include <initializer_list>
#include <type_traits>
#include <vector>

constexpr int BASE_WIDTH = 640;
constexpr int BASE_HEIGHT = 480;

inline int g_width = 960;
inline int g_height = 720;

inline HWND g_gameWnd = nullptr;

inline double scaleX() {
    return g_width / static_cast<double>(BASE_WIDTH);
}

inline double scaleY() {
    return g_height / static_cast<double>(BASE_HEIGHT);
}

inline long scaleFromBaseX(long value) {
    return lround(value * scaleX());
}

inline long scaleFromBaseY(long value) {
    return lround(value * scaleY());
}

inline long scaleToBaseX(long value) {
    return lround(value / scaleX());
}

inline long scaleToBaseY(long value) {
    return lround(value / scaleY());
}

inline bool GameHasFocus() {
    return g_gameWnd && GetForegroundWindow() == g_gameWnd;
}

inline bool writeCode(void* address, const void* bytes, size_t length) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memcpy(address, bytes, length);
    VirtualProtect(address, length, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), address, length);
    return true;
}

inline bool nopCode(void* address, size_t length) {
    std::vector<BYTE> nops(length, 0x90);
    return writeCode(address, nops.data(), length);
}

// Hooks the first signature that matches in the game executable.
template <typename Fn>
inline bool installSignatureHook(HookHolder<Fn>& hook,
                                 const char* name,
                                 std::initializer_list<MemorySignature> signatures,
                                 std::type_identity_t<Fn> handler) {
    CSigScan scanner(nullptr);
    for (const MemorySignature& signature : signatures) {
        void* address = scanner.Scan(signature);
        if (address) {
            std::cout << "[patch] " << name << " found at " << address << std::endl;
            hook = HookFactory::install(address, handler);
            return true;
        }
    }

    std::cout << "[patch] " << name << " not found, skipping..." << std::endl;
    return false;
}

inline DWORD g_firstSeen = 0;
inline bool g_childFixed = false;
