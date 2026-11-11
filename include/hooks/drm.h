#pragma once

#include "pch.h"
#include <hooks/shared.h>
#include <algorithm>
#include <string>
#include <vector>

struct DrmPatch {
    const char* name;
    MemorySignature signature;
    void (*apply)(BYTE* match);
    int applied = 0;
};

// When no InstalledSKUs/SKUInfo.ini entry exists, the game falls back to the "TESTDEV0" developer
// SKU. So just go make it fall back lol
constexpr MemorySignature FIND_SKU_FALLBACK_SIGNATURE = {
    "\x3B\xC3\x89\x5D\xFC\x74\x00\x80\x7D\x00\x01\x75\x00\x8B\x45\x00\x8A\x48\x00\x8B\x75\x08\x80"
    "\xC9\x01\x88\x48\x00\x68\x00\x00\x00\x00",
    "xxxxxx?xx?xx?xx?xx?xxxxxxxx?x????",
    33,
};

inline void applyFindSkuFallback(BYTE* match) {
    nopCode(match + 0x05, 2);
    nopCode(match + 0x0B, 2);
}

// RegQueryValueExA failing throws "Unable to get the value from the registry".
constexpr MemorySignature REGISTRY_ATTRIBUTE_THROW_SIGNATURE = {
    "\xFF\x15\x00\x00\x00\x00\x3B\xC3\x74\x00\x68\x00\x00\x00\x00\x8D\x4D\x00\x51\xC7\x45\x00\x00"
    "\x00\x00\x00\xE8\x00\x00\x00\x00\x8D\x85\x00\x00\x00\x00",
    "xx????xxx?x????xx?xxx?????x????xx????",
    37,
};

inline void applyRegistryAttributeDefault(BYTE* match) {
    int32_t bufferDisplacement = *reinterpret_cast<int32_t*>(match + 0x21);

    // test eax, eax / je +7 / mov byte ptr [ebp+disp32], 0 / nop padding over the old throw
    BYTE code[25];
    memset(code, 0x90, sizeof(code));
    const BYTE prefix[] = {0x85, 0xC0, 0x74, 0x07, 0xC6, 0x85};
    memcpy(code, prefix, sizeof(prefix));
    memcpy(code + sizeof(prefix), &bufferDisplacement, sizeof(bufferDisplacement));
    code[sizeof(prefix) + sizeof(bufferDisplacement)] = 0x00;

    writeCode(match + 0x06, code, sizeof(code));
}

// CRuntimeAttribute::GetBool has blank values throwing an error
constexpr MemorySignature GET_BOOL_THROW_SIGNATURE = {
    "\x6A\x05\x68\x00\x00\x00\x00\x50\x6A\x00\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x85\xC0\x75\x00",
    "xxx????xxxxxx?x????xxx?",
    23,
};

inline void applyGetBoolDefault(BYTE* match) {
    nopCode(match + 0x15, 2);
}

inline DrmPatch drmPatches[] = {
    {"FindSKU fallback", FIND_SKU_FALLBACK_SIGNATURE, &applyFindSkuFallback},
    {"Registry default", REGISTRY_ATTRIBUTE_THROW_SIGNATURE, &applyRegistryAttributeDefault},
    {"GetBool default", GET_BOOL_THROW_SIGNATURE, &applyGetBoolDefault},
};

inline void installDrmPatches() {
    CSigScan scanner(nullptr);
    for (DrmPatch& patch : drmPatches) {
        for (void* match : scanner.ScanAll(patch.signature)) {
            BYTE* code = static_cast<BYTE*>(match);

            patch.apply(code);
            patch.applied++;
        }

        if (patch.applied) {
            std::cout << std::format("[drm] Applied {} ({} site(s))...", patch.name, patch.applied)
                      << std::endl;
        } else {
            std::cout << std::format("[drm] {} not found, skipping...", patch.name) << std::endl;
        }
    }
}