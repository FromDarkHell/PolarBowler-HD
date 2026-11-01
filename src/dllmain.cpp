#include "pch.h"
#include <HookLib.h>
#include <INIReader.h>

#include "hooks/resolution.h"
#include "hooks/input.h"
#include "hooks/webdriver.h"
#include <filesystem>

HMODULE self = NULL;

DWORD WINAPI executionThread(LPTHREAD_START_ROUTINE)
{
    AllocConsole();
    FILE *fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);

    std::cout << "PolarBowler-HD Started..." << std::endl;

    std::cout << "Reading PolarBowler-HD Config..." << std::endl;
    char modulePath[MAX_PATH];
    GetModuleFileNameA(self, modulePath, MAX_PATH);

    std::filesystem::path configPath = std::filesystem::path(modulePath).replace_filename("PolarBowler-HD.ini");
    std::cout << "Checking for INI file at '" << configPath.string() << "'" << std::endl;
    if (std::filesystem::exists(configPath))
    {
        INIReader reader(configPath.string());

        g_height = reader.GetInteger("Display", "Height", g_height);
        g_width = reader.GetInteger("Display", "Width", g_width);
    }

    std::cout << "Installing hooks..." << std::endl;

    installResolutionHooks();
    installWebDriverHooks();
    installInputHooks();

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        self = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)executionThread, NULL, NULL, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
