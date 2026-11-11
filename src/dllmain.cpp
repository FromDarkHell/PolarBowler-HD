#include "pch.h"
#include <HookLib.h>
#include <INIReader.h>

#include "hooks/drm.h"
#include "hooks/input.h"
#include "hooks/resolution.h"
#include "hooks/webdriver.h"
#include "util/dump.h"

#include <filesystem>

HMODULE self = NULL;

DWORD WINAPI execute() {
#ifdef _DEBUG
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif

    std::cout << "PolarBowler-HD Started..." << std::endl;

    std::cout << "Reading PolarBowler-HD Config..." << std::endl;
    char modulePath[MAX_PATH];
    GetModuleFileNameA(self, modulePath, MAX_PATH);

    std::filesystem::path configPath =
        std::filesystem::path(modulePath).replace_filename("PolarBowler-HD.ini");
    std::cout << "Checking for INI file at '" << configPath.string() << "'" << std::endl;
    if (std::filesystem::exists(configPath)) {
        INIReader reader(configPath.string());

        g_height = reader.GetInteger("Display", "Height", g_height);
        g_width = reader.GetInteger("Display", "Width", g_width);
    }

    dumpUnwrappedExecutable();

    std::cout << "Installing hooks..." << std::endl;

    installDrmPatches();
    installResolutionHooks();
    installWebDriverHooks();
    installInputHooks();

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            self = hModule;
            DisableThreadLibraryCalls(hModule);
            execute();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
