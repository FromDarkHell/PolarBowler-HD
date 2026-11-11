#pragma once

#include "pch.h"
#include <HookLib.h>
#include <hooks/shared.h>

// Camera::addDrop wrapper, only present in Polar Bowler Classic.exe (0x00404730).
constexpr MemorySignature CAMERA_ADD_DROP_SIGNATURE = {
    "\x51\x8B\x54\x24\x10\x56\x8B\xF1\x8D\x4C\x24\x04\x51\x8B\x4C\x24\x14\x52\xC7\x44\x24\x0C\x00"
    "\x00\x00\x00\x8B\x06\x51\x56\xC7\x44\x24\x14\x00\x00\x00\x00\xFF\x90\xA8\x01\x00\x00",
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    44,
};

// Byte offsets into the IWTDrop COM vtable.
enum class DropVtable : size_t {
    GET_WIDTH = 0xB4,
    GET_HEIGHT = 0xB8,
    GET_X = 0xBC,
    GET_Y = 0xC0,
    GET_BITMAP_WIDTH = 0xC4,
    GET_BITMAP_HEIGHT = 0xC8,
    SET_SIZE = 0xDC,
    SET_POSITION = 0xE0,
    SET_FILTERING = 0x108,
};

using DropSetValueFn = HRESULT(__stdcall*)(void*, long);
using DropSetValuesFn = HRESULT(__stdcall*)(void*, long, long);
using DropGetValueFn = HRESULT(__stdcall*)(void*, long*);

// Camera::addDrop is a __thiscall that returns IWTDrop** through a hidden result pointer.
using CameraAddDropFn =
    void**(__fastcall*)(void* camera, void* edx, void** result, void* bitmap, int front);

inline HookHolder<CameraAddDropFn> cameraAddDropHook;
inline HookHolder<DropSetValuesFn> dropSetSizeHook;
inline HookHolder<DropSetValuesFn> dropSetPositionHook;
inline HookHolder<DropGetValueFn> dropGetWidthHook;
inline HookHolder<DropGetValueFn> dropGetHeightHook;
inline HookHolder<DropGetValueFn> dropGetXHook;
inline HookHolder<DropGetValueFn> dropGetYHook;

inline void* vtableEntry(void* object, DropVtable offset) {
    void** vtable = *reinterpret_cast<void***>(object);
    return vtable[static_cast<size_t>(offset) / sizeof(void*)];
}

// The game reads drop geometry back in 640x480 space. So this needs to scale it back down for the
// sake of it.
inline HRESULT unscaleResult(HRESULT result, long* value, long (*scaleToBase)(long)) {
    if (SUCCEEDED(result) && value) {
        *value = scaleToBase(*value);
    }
    return result;
}

inline HRESULT __stdcall dropSetSizeHandler(void* drop, long width, long height) {
    return dropSetSizeHook.call(drop, scaleFromBaseX(width), scaleFromBaseY(height));
}

inline HRESULT __stdcall dropSetPositionHandler(void* drop, long x, long y) {
    return dropSetPositionHook.call(drop, scaleFromBaseX(x), scaleFromBaseY(y));
}

inline HRESULT __stdcall dropGetWidthHandler(void* drop, long* width) {
    return unscaleResult(dropGetWidthHook.call(drop, width), width, scaleToBaseX);
}

inline HRESULT __stdcall dropGetHeightHandler(void* drop, long* height) {
    return unscaleResult(dropGetHeightHook.call(drop, height), height, scaleToBaseY);
}

inline HRESULT __stdcall dropGetXHandler(void* drop, long* x) {
    return unscaleResult(dropGetXHook.call(drop, x), x, scaleToBaseX);
}

inline HRESULT __stdcall dropGetYHandler(void* drop, long* y) {
    return unscaleResult(dropGetYHook.call(drop, y), y, scaleToBaseY);
}

// IWTDrop is implemented inside WebDriver.
// Can only reach its vtable once the game creates its first drop.
inline void installDropHooks(void* drop) {
    if (dropSetSizeHook.valid()) {
        return;
    }

    dropSetSizeHook =
        HookFactory::install(vtableEntry(drop, DropVtable::SET_SIZE), &dropSetSizeHandler);
    dropSetPositionHook =
        HookFactory::install(vtableEntry(drop, DropVtable::SET_POSITION), &dropSetPositionHandler);
    dropGetWidthHook =
        HookFactory::install(vtableEntry(drop, DropVtable::GET_WIDTH), &dropGetWidthHandler);
    dropGetHeightHook =
        HookFactory::install(vtableEntry(drop, DropVtable::GET_HEIGHT), &dropGetHeightHandler);
    dropGetXHook = HookFactory::install(vtableEntry(drop, DropVtable::GET_X), &dropGetXHandler);
    dropGetYHook = HookFactory::install(vtableEntry(drop, DropVtable::GET_Y), &dropGetYHandler);

    std::cout << "[webdriver] Installed IWTDrop hooks..." << std::endl;
}

inline void scaleNewDrop(void* drop) {
    if (!dropSetSizeHook.active() || !dropSetPositionHook.active() || !dropGetXHook.active()
        || !dropGetYHook.active()) {
        return;
    }

    long bitmapWidth = 0, bitmapHeight = 0;
    reinterpret_cast<DropGetValueFn>(vtableEntry(drop, DropVtable::GET_BITMAP_WIDTH))(drop,
                                                                                      &bitmapWidth);
    reinterpret_cast<DropGetValueFn>(vtableEntry(drop, DropVtable::GET_BITMAP_HEIGHT))(
        drop, &bitmapHeight);

    // Get the unscaled X/Y values first, and then scale them up
    long x = 0, y = 0;
    dropGetXHook.call(drop, &x);
    dropGetYHook.call(drop, &y);

    dropSetSizeHook.call(drop, scaleFromBaseX(bitmapWidth), scaleFromBaseY(bitmapHeight));
    dropSetPositionHook.call(drop, scaleFromBaseX(x), scaleFromBaseY(y));

    // Bilinear filtering so the upscaled bitmap isn't blocky.
    reinterpret_cast<DropSetValueFn>(vtableEntry(drop, DropVtable::SET_FILTERING))(drop, 1);
}

inline void** __fastcall cameraAddDropHandler(void* camera,
                                              void* edx,
                                              void** result,
                                              void* bitmap,
                                              int front) {
    void** dropSlot = cameraAddDropHook.call(camera, edx, result, bitmap, front);
    if (dropSlot && *dropSlot) {
        installDropHooks(*dropSlot);
        scaleNewDrop(*dropSlot);
    }

    return dropSlot;
}

inline void installWebDriverHooks() {
    installSignatureHook(cameraAddDropHook, "Camera::addDrop", {CAMERA_ADD_DROP_SIGNATURE},
                         &cameraAddDropHandler);
}
