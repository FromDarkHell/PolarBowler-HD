#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

#include "scanning/signature.h"

// Based off CSigScan from AlliedModders

struct MemorySignature;

class CSigScan {
   private:
    char* m_pModuleBase = nullptr;
    size_t m_moduleLen = 0;

   public:
    HMODULE m_moduleHandle = nullptr;
    CSigScan(const wchar_t* moduleName) {
        m_moduleHandle = GetModuleHandle(moduleName);
        if (m_moduleHandle == nullptr) {
            return;
        }

        void* pAddr = m_moduleHandle;

        MEMORY_BASIC_INFORMATION mem;
        if (!VirtualQuery(pAddr, &mem, sizeof(mem))) {
            return;
        }

        m_pModuleBase = (char*)mem.AllocationBase;
        if (m_pModuleBase == nullptr) {
            return;
        }

        IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)mem.AllocationBase;
        IMAGE_NT_HEADERS* pe =
            (IMAGE_NT_HEADERS*)((unsigned long)dos + (unsigned long)dos->e_lfanew);

        m_moduleLen = pe->OptionalHeader.SizeOfImage;
    }

    void* Scan(const MemorySignature& sigStruct) {
        return Scan(sigStruct.Sig, sigStruct.Mask, sigStruct.Length);
    }

    std::vector<void*> ScanAll(const MemorySignature& sigStruct) {
        std::vector<void*> matches;
        char* start = m_pModuleBase;
        while (void* match = Scan(sigStruct.Sig, sigStruct.Mask, sigStruct.Length, start)) {
            matches.push_back(match);
            start = static_cast<char*>(match) + 1;
        }
        return matches;
    }

    char* ModuleBase() const { return m_pModuleBase; }

    size_t ModuleLength() const { return m_moduleLen; }

    void* Scan(const char* sig, const char* mask, int sigLength, char* start = nullptr) {
        if (m_pModuleBase == nullptr) {
            return nullptr;
        }

        char* pData = start ? start : m_pModuleBase;
        char* pEnd = m_pModuleBase + m_moduleLen;

        while (pData + sigLength <= pEnd) {
            int i;
            for (i = 0; i < sigLength; i++) {
                if (mask[i] != '?' && sig[i] != pData[i]) {
                    break;
                }
            }

            // The for loop finished on its own accord
            if (i == sigLength) {
                return (void*)pData;
            }

            pData++;
        }
        return nullptr;
    }

    void* Scan(const char* sig, const char* mask) {
        int sigLength = strlen(mask);
        return Scan(sig, mask, sigLength);
    }
};