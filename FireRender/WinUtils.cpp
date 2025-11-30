#include "pch.h"
#include "WinUtils.hpp"

#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

void CreateMiniDump(EXCEPTION_POINTERS* e)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    char filename[MAX_PATH];
    sprintf_s(filename, "Crash_%04d%02d%02d_%02d%02d%02d.dmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);

    if ((hFile != nullptr) && (hFile != INVALID_HANDLE_VALUE)) {
        MINIDUMP_EXCEPTION_INFORMATION info;
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = e;
        info.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpWithFullMemory, // or MiniDumpNormal
            e ? &info : nullptr,
            nullptr,
            nullptr
        );

        CloseHandle(hFile);
        std::cout << "Crash dump written: " << filename << std::endl;
    }
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* e)
{
    std::cerr << "Unhandled exception caught! Creating dump...\n";
    CreateMiniDump(e);
    return EXCEPTION_EXECUTE_HANDLER;
}


void WinUtils::Init()
{
    SetUnhandledExceptionFilter(ExceptionHandler);
}

