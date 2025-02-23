#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

class Memory {
public:
    Memory(const std::string& processName) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(entry);

        if (Process32First(snapshot, &entry)) {
            do {
                if (_stricmp(entry.szExeFile, processName.c_str()) == 0) {
                    processId = entry.th32ProcessID;
                    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                    break;
                }
            } while (Process32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }

    ~Memory() {
        if (handle) CloseHandle(handle);
    }

    uintptr_t GetModuleBase(const std::string& moduleName) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;

        MODULEENTRY32 entry;
        entry.dwSize = sizeof(entry);
        uintptr_t baseAddress = 0;

        if (Module32First(snapshot, &entry)) {
            do {
                if (_stricmp(entry.szModule, moduleName.c_str()) == 0) {
                    baseAddress = (uintptr_t)entry.modBaseAddr;
                    break;
                }
            } while (Module32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
        return baseAddress;
    }

    template <typename T>
    T ReadMemory(uintptr_t address) {
        T buffer;
        ReadProcessMemory(handle, (LPCVOID)address, &buffer, sizeof(T), nullptr);
        return buffer;
    }

    template <typename T>
    void WriteMemory(uintptr_t address, T value) {
        WriteProcessMemory(handle, (LPVOID)address, &value, sizeof(T), nullptr);
    }

private:
    HANDLE handle = nullptr;
    DWORD processId = 0;
};
