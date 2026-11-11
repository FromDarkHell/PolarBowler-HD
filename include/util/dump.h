inline std::string g_drmDumpStatus;

inline bool readWholeFile(const wchar_t* path, std::vector<BYTE>& contents) {
    HANDLE file =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < 0x10000000;
    if (ok) {
        contents.resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        ok = ReadFile(file, contents.data(), static_cast<DWORD>(contents.size()), &read, nullptr)
             && read == contents.size();
    }
    CloseHandle(file);
    return ok;
}

inline size_t countNonZero(const BYTE* data, size_t length) {
    size_t count = 0;
    for (size_t i = 0; i < length; i++) {
        count += data[i] != 0;
    }
    return count;
}

// The image is wrapped if the on-disk code is mostly empty, but loaded has code in it
inline bool isHollowedImage(const BYTE* image, const std::vector<BYTE>& disk) {
    if (disk.size() < sizeof(IMAGE_DOS_HEADER)) {
        return false;
    }

    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(disk.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0
        || static_cast<size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS) > disk.size()) {
        return false;
    }

    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(disk.data() + dos->e_lfanew);
    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    size_t diskCode = 0;
    size_t memoryCode = 0;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
        if (!(section->Characteristics & IMAGE_SCN_CNT_CODE)) {
            continue;
        }

        size_t length = (std::min<size_t>)(section->SizeOfRawData, section->Misc.VirtualSize);
        if (static_cast<size_t>(section->PointerToRawData) + length > disk.size()) {
            return false;
        }

        diskCode += countNonZero(disk.data() + section->PointerToRawData, length);
        memoryCode += countNonZero(image + section->VirtualAddress, section->Misc.VirtualSize);
    }

    return memoryCode > 0x1000 && diskCode * 4 < memoryCode;
}

inline void copyReadableImage(const BYTE* base, size_t size, std::vector<BYTE>& out) {
    out.assign(size, 0);
    size_t offset = 0;
    while (offset < size) {
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(base + offset, &region, sizeof(region))) {
            break;
        }

        size_t regionEnd = static_cast<const BYTE*>(region.BaseAddress) - base + region.RegionSize;
        size_t length = (std::min<size_t>)(regionEnd, size) - offset;
        bool readable =
            region.State == MEM_COMMIT && !(region.Protect & (PAGE_NOACCESS | PAGE_GUARD));
        if (readable) {
            memcpy(out.data() + offset, base + offset, length);
        }

        offset += length;
    }
}

// Rebuilds the image and the IAT so Windows imports properly
inline bool rebuildImage(std::vector<BYTE>& image, size_t& unresolvedImports) {
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.data());
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(image.data() + dos->e_lfanew);
    IMAGE_OPTIONAL_HEADER& optional = nt->OptionalHeader;
    DWORD alignment = optional.SectionAlignment;

    optional.FileAlignment = alignment;
    optional.CheckSum = 0;
    IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
        section->PointerToRawData = section->VirtualAddress;
        section->SizeOfRawData =
            (section->Misc.VirtualSize + alignment - 1) / alignment * alignment;
    }

    optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] = {};

    unresolvedImports = 0;
    const IMAGE_DATA_DIRECTORY& importDirectory =
        optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    for (DWORD offset = importDirectory.VirtualAddress;
         offset && offset + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= image.size();
         offset += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image.data() + offset);
        if (!descriptor->Name) {
            break;
        }

        descriptor->TimeDateStamp = 0;
        descriptor->ForwarderChain = 0;
        if (!descriptor->OriginalFirstThunk) {
            unresolvedImports++;
            continue;
        }

        for (DWORD index = 0;; index++) {
            size_t nameThunk = descriptor->OriginalFirstThunk + index * sizeof(IMAGE_THUNK_DATA);
            size_t addressThunk = descriptor->FirstThunk + index * sizeof(IMAGE_THUNK_DATA);
            if (nameThunk + sizeof(IMAGE_THUNK_DATA) > image.size()
                || addressThunk + sizeof(IMAGE_THUNK_DATA) > image.size()) {
                return false;
            }

            memcpy(image.data() + addressThunk, image.data() + nameThunk, sizeof(IMAGE_THUNK_DATA));
            if (!reinterpret_cast<IMAGE_THUNK_DATA*>(image.data() + nameThunk)->u1.AddressOfData) {
                break;
            }
        }
    }
    return true;
}

// Writes an unwrapped exe next to the current one
inline void dumpUnwrappedExecutable() {
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH)) {
        return;
    }

    std::vector<BYTE> disk;
    if (!readWholeFile(exePath, disk)) {
        std::cerr << "could not read the exe from disk" << std::endl;
        return;
    }

    auto base = reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr));
    if (!isHollowedImage(base, disk)) {
        return;
    }

    std::wstring outputPath = exePath;
    size_t extension = outputPath.find_last_of(L'.');
    size_t separator = outputPath.find_last_of(L"\\/");
    if (extension == std::wstring::npos
        || (separator != std::wstring::npos && extension < separator)) {
        extension = outputPath.size();
    }
    outputPath.insert(extension, L"-unwrapped");

    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
    std::vector<BYTE> image;
    copyReadableImage(base, nt->OptionalHeader.SizeOfImage, image);

    size_t unresolvedImports = 0;
    if (!rebuildImage(image, unresolvedImports)) {
        std::cerr << "import table is malformed, not dumping" << std::endl;
        return;
    }

    HANDLE file = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        std::cerr << (GetLastError() == ERROR_FILE_EXISTS
                          ? "unwrapped exe already exists, not overwriting"
                          : std::format("could not create unwrapped exe (error {})",
                                        GetLastError()))
                  << std::endl;
        return;
    }

    DWORD written = 0;
    bool ok = WriteFile(file, image.data(), static_cast<DWORD>(image.size()), &written, nullptr)
              && written == image.size();
    CloseHandle(file);

    char narrowPath[MAX_PATH * 2]{};
    WideCharToMultiByte(CP_UTF8, 0, outputPath.c_str(), -1, narrowPath, sizeof(narrowPath), nullptr,
                        nullptr);
    if (!ok) {
        std::cerr << std::format("failed writing '{}'", narrowPath) << std::endl;
    } else if (unresolvedImports) {
        std::cerr << std::format(
            "wrote '{}', but {} import(s) have no name table and keep wrapper addresses",
            narrowPath, unresolvedImports)
                  << std::endl;
    } else {
        std::cout << std::format("wrote '{}' ({} bytes)", narrowPath, image.size()) << std::endl;
    }
}
