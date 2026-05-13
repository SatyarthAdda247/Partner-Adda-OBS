// AddaPartner.exe - Launcher for Adda Partner (OBS Studio)
// Shows as "Adda Partner" in taskbar/Task Manager instead of "OBS Studio"
// Searches for obs64.exe in multiple locations

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static bool TryLaunch(const wchar_t *obsPath, const wchar_t *workDir)
{
    if (!obsPath || obsPath[0] == L'\0')
        return false;

    // Check file exists first
    DWORD attr = GetFileAttributesW(obsPath);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;

    wchar_t cmdLine[MAX_PATH * 2] = {};
    wcscpy_s(cmdLine, L"\"");
    wcscat_s(cmdLine, obsPath);
    wcscat_s(cmdLine, L"\"");

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessW(
        obsPath,
        cmdLine,
        nullptr, nullptr,
        FALSE, 0,
        nullptr,
        workDir,
        &si, &pi
    );

    if (ok) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // --- Location 1: Same directory as AddaPartner.exe ---
    wchar_t selfPath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

    wchar_t selfDir[MAX_PATH] = {};
    wcscpy_s(selfDir, selfPath);
    wchar_t *lastSlash = wcsrchr(selfDir, L'\\');
    if (lastSlash) *lastSlash = L'\0';

    wchar_t obs1[MAX_PATH] = {};
    wcscpy_s(obs1, selfDir);
    wcscat_s(obs1, L"\\obs64.exe");

    if (TryLaunch(obs1, selfDir))
        return 0;

    // --- Location 2: Standard OBS install path ---
    wchar_t obs2[MAX_PATH] = {};
    ExpandEnvironmentStringsW(
        L"%ProgramFiles%\\obs-studio\\bin\\64bit\\obs64.exe",
        obs2, MAX_PATH);

    wchar_t obsDir2[MAX_PATH] = {};
    wcscpy_s(obsDir2, obs2);
    wchar_t *sl2 = wcsrchr(obsDir2, L'\\');
    if (sl2) *sl2 = L'\0';

    if (TryLaunch(obs2, obsDir2))
        return 0;

    // --- Location 3: ProgramFiles(x86) fallback ---
    wchar_t obs3[MAX_PATH] = {};
    ExpandEnvironmentStringsW(
        L"%ProgramFiles(x86)%\\obs-studio\\bin\\64bit\\obs64.exe",
        obs3, MAX_PATH);

    wchar_t obsDir3[MAX_PATH] = {};
    wcscpy_s(obsDir3, obs3);
    wchar_t *sl3 = wcsrchr(obsDir3, L'\\');
    if (sl3) *sl3 = L'\0';

    if (TryLaunch(obs3, obsDir3))
        return 0;

    // All locations failed - build a detailed error message
    wchar_t msg[2048] = {};
    _snwprintf_s(msg, _countof(msg), _TRUNCATE,
        L"Failed to launch Adda Partner.\n\n"
        L"Could not find obs64.exe in any of these locations:\n\n"
        L"  1. %s\n"
        L"  2. %s\n"
        L"  3. %s\n\n"
        L"Please reinstall OBS Studio or Adda Partner.\n"
        L"Download OBS Studio from: https://obsproject.com",
        obs1, obs2, obs3);

    MessageBoxW(nullptr, msg, L"Adda Partner", MB_ICONERROR | MB_OK);
    return 1;
}
