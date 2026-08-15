// injector.cpp : Standalone injector for WaWDll.
//
// Waits for Call of Duty: World at War to show up and loads WaWDll.dll into it
// using the classic CreateRemoteThread(LoadLibraryA) technique.
//
// The game is 32-bit, so this injector must be built as 32-bit as well. The
// trick below relies on kernel32.dll sitting at the same base address in every
// process of the same bitness, which is only true if we match the target.

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <cstdio>
#include <cstdlib>

namespace
{
    // World at War ships separate executables for the solo campaign / Nazi
    // Zombies and for multiplayer. WaWDll targets the solo one.
    const char *const DEFAULT_PROCESS = "CoDWaW.exe";
    const char *const DEFAULT_DLL = "WaWDll.dll";

    struct Options
    {
        std::string process = DEFAULT_PROCESS;
        std::string dll;                    // Empty = search next to this exe
        std::string launch;                 // Command line to start first
        DWORD delayMs = 5000;               // Settle time before injecting
        DWORD timeoutMs = 0;                // 0 = wait for the game forever
        bool watch = false;                 // Keep running for further launches
    };

    std::string FormatError(DWORD code)
    {
        char *buffer = nullptr;
        DWORD written = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, code, 0, (LPSTR)&buffer, 0, nullptr);

        std::string message = (written && buffer) ? buffer : "unknown error";
        if (buffer)
            LocalFree(buffer);

        while (!message.empty() &&
            (message.back() == '\n' || message.back() == '\r'))
            message.pop_back();

        char suffix[32] = { 0 };
        sprintf_s(suffix, " (0x%08X)", code);
        return message + suffix;
    }

    bool EqualsNoCase(const char *a, const std::string &b)
    {
        return _stricmp(a, b.c_str()) == 0;
    }

    // Needed when the game runs elevated, for example when Steam does.
    bool EnableDebugPrivilege()
    {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            return false;

        TOKEN_PRIVILEGES privileges = { 0 };
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        bool enabled = LookupPrivilegeValueA(nullptr, SE_DEBUG_NAME,
            &privileges.Privileges[0].Luid) != FALSE;
        if (enabled)
            enabled = AdjustTokenPrivileges(token, FALSE, &privileges,
                sizeof(privileges), nullptr, nullptr) != FALSE &&
                GetLastError() == ERROR_SUCCESS;

        CloseHandle(token);
        return enabled;
    }

    DWORD FindProcess(const std::string &name)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32 entry = { sizeof(entry) };
        DWORD pid = 0;
        if (Process32First(snapshot, &entry))
        {
            do
            {
                if (EqualsNoCase(entry.szExeFile, name))
                {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return pid;
    }

    bool IsModuleLoaded(DWORD pid, const std::string &moduleName)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE)
            return false;

        MODULEENTRY32 entry = { sizeof(entry) };
        bool found = false;
        if (Module32First(snapshot, &entry))
        {
            do
            {
                if (EqualsNoCase(entry.szModule, moduleName))
                {
                    found = true;
                    break;
                }
            } while (Module32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return found;
    }

    // The injector is 32-bit, so the target has to be a WOW64 process for our
    // kernel32 addresses to mean anything inside it.
    bool IsTargetCompatible(HANDLE process)
    {
        BOOL selfWow64 = FALSE;
        BOOL targetWow64 = FALSE;
        if (!IsWow64Process(GetCurrentProcess(), &selfWow64) ||
            !IsWow64Process(process, &targetWow64))
            return true;    // Cannot tell, so assume the caller knows better

        return selfWow64 == targetWow64;
    }

    bool Inject(HANDLE process, const std::string &dllPath)
    {
        const SIZE_T size = dllPath.size() + 1;

        LPVOID remotePath = VirtualAllocEx(process, nullptr, size,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath)
        {
            printf("[-] VirtualAllocEx failed: %s\n",
                FormatError(GetLastError()).c_str());
            return false;
        }

        bool injected = false;
        bool threadFinished = false;

        do
        {
            if (!WriteProcessMemory(process, remotePath, dllPath.c_str(),
                size, nullptr))
            {
                printf("[-] WriteProcessMemory failed: %s\n",
                    FormatError(GetLastError()).c_str());
                break;
            }

            LPTHREAD_START_ROUTINE loadLibrary = (LPTHREAD_START_ROUTINE)
                GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
            if (!loadLibrary)
            {
                printf("[-] Could not resolve LoadLibraryA: %s\n",
                    FormatError(GetLastError()).c_str());
                break;
            }

            HANDLE thread = CreateRemoteThread(process, nullptr, 0,
                loadLibrary, remotePath, 0, nullptr);
            if (!thread)
            {
                printf("[-] CreateRemoteThread failed: %s\n",
                    FormatError(GetLastError()).c_str());
                break;
            }

            // WaWDll installs nine detours from DllMain, so give it room.
            DWORD moduleBase = 0;
            if (WaitForSingleObject(thread, 30000) == WAIT_OBJECT_0)
            {
                threadFinished = true;
                GetExitCodeThread(thread, &moduleBase);
            }
            CloseHandle(thread);

            if (!threadFinished)
            {
                printf("[-] The remote LoadLibraryA call did not return in "
                    "time. The game may be hung or still loading.\n");
                break;
            }

            if (!moduleBase)
            {
                printf("[-] LoadLibraryA returned NULL inside the game. The "
                    "DLL was found but could not be loaded.\n");
                break;
            }

            printf("[+] Loaded at 0x%08X\n", moduleBase);
            injected = true;
        } while (false);

        // Only safe to reclaim once we know nothing is still reading it.
        if (threadFinished)
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);

        return injected;
    }

    std::string DirectoryOfThisExe()
    {
        char path[MAX_PATH] = { 0 };
        DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (!length)
            return std::string();

        std::string full(path, length);
        size_t slash = full.find_last_of("\\/");
        return slash == std::string::npos
            ? std::string() : full.substr(0, slash + 1);
    }

    bool FileExists(const std::string &path)
    {
        DWORD attributes = GetFileAttributesA(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES &&
            !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    std::string ToAbsolute(const std::string &path)
    {
        char resolved[MAX_PATH] = { 0 };
        DWORD length = GetFullPathNameA(path.c_str(), MAX_PATH, resolved, nullptr);
        return (length && length < MAX_PATH)
            ? std::string(resolved, length) : path;
    }

    // Looks beside the injector first, then in the sibling build output
    // folders, so it works straight out of the build tree.
    std::string LocateDll(const std::string &requested)
    {
        if (!requested.empty())
            return ToAbsolute(requested);

        const std::string here = DirectoryOfThisExe();
        const char *const candidates[] =
        {
            "",
            "..\\Debug\\",
            "..\\Release\\",
        };

        for (const char *relative : candidates)
        {
            std::string candidate = here + relative + DEFAULT_DLL;
            if (FileExists(candidate))
                return ToAbsolute(candidate);
        }

        return ToAbsolute(here + DEFAULT_DLL);
    }

    void PrintUsage()
    {
        printf(
            "WaWDll injector\n"
            "\n"
            "Usage: WaWInjector.exe [options] [-- <command to launch>]\n"
            "\n"
            "  -d, --dll <path>      DLL to inject (default: WaWDll.dll beside\n"
            "                        this exe, or ..\\Debug\\WaWDll.dll)\n"
            "  -p, --process <name>  Process to inject into (default: %s)\n"
            "  -w, --wait <ms>       Settle time after the game is ready but\n"
            "                        before injecting (default: 5000)\n"
            "  -t, --timeout <ms>    Give up if the game does not start in time\n"
            "                        (default: wait forever)\n"
            "      --watch           Stay resident and inject on every launch\n"
            "  -h, --help            Show this help\n"
            "\n"
            "Everything after -- is launched first, then injected into. That is\n"
            "what makes this usable as a Steam launch option:\n"
            "\n"
            "  \"C:\\path\\to\\WaWInjector.exe\" -- %%command%%\n"
            "\n"
            "With no -- it simply waits for %s to appear, so you can start it\n"
            "beforehand and launch the game however you like.\n",
            DEFAULT_PROCESS, DEFAULT_PROCESS);
    }

    bool ParseArgs(int argc, char **argv, Options &options)
    {
        for (int i = 1; i < argc; i++)
        {
            const std::string arg = argv[i];
            const bool hasValue = (i + 1) < argc;

            if (arg == "-h" || arg == "--help" || arg == "/?")
                return false;
            else if ((arg == "-d" || arg == "--dll") && hasValue)
                options.dll = argv[++i];
            else if ((arg == "-p" || arg == "--process") && hasValue)
                options.process = argv[++i];
            else if ((arg == "-w" || arg == "--wait") && hasValue)
                options.delayMs = (DWORD)strtoul(argv[++i], nullptr, 10);
            else if ((arg == "-t" || arg == "--timeout") && hasValue)
                options.timeoutMs = (DWORD)strtoul(argv[++i], nullptr, 10);
            else if (arg == "--watch")
                options.watch = true;
            else if (arg == "--")
            {
                // The rest of the line is the command to launch. Re-quote any
                // argument containing spaces so Steam's %command% survives the
                // round trip through argv.
                for (int j = i + 1; j < argc; j++)
                {
                    const std::string part = argv[j];
                    if (!options.launch.empty())
                        options.launch += ' ';

                    if (part.find(' ') != std::string::npos &&
                        part.find('"') == std::string::npos)
                        options.launch += '"' + part + '"';
                    else
                        options.launch += part;
                }
                break;
            }
            else
            {
                printf("[-] Unrecognized argument: %s\n\n", arg.c_str());
                return false;
            }
        }

        return true;
    }

    bool LaunchGame(const std::string &commandLine)
    {
        printf("[*] Launching: %s\n", commandLine.c_str());

        // CreateProcessA is allowed to write to its command line buffer.
        std::string mutableCommandLine = commandLine;
        mutableCommandLine.push_back('\0');

        STARTUPINFOA startup = { sizeof(startup) };
        PROCESS_INFORMATION info = { 0 };

        if (!CreateProcessA(nullptr, &mutableCommandLine[0], nullptr, nullptr,
            FALSE, 0, nullptr, nullptr, &startup, &info))
        {
            printf("[-] Could not start the game: %s\n",
                FormatError(GetLastError()).c_str());
            return false;
        }

        CloseHandle(info.hThread);
        CloseHandle(info.hProcess);
        return true;
    }

    // Returns 0 on timeout.
    DWORD WaitForGame(const Options &options)
    {
        const DWORD started = GetTickCount();
        bool announced = false;

        for (;;)
        {
            DWORD pid = FindProcess(options.process);
            if (pid)
                return pid;

            if (options.timeoutMs &&
                (GetTickCount() - started) >= options.timeoutMs)
                return 0;

            if (!announced)
            {
                printf("[*] Waiting for %s ...\n", options.process.c_str());
                announced = true;
            }

            Sleep(250);
        }
    }

    bool InjectIntoGame(DWORD pid, const Options &options,
        const std::string &dllPath)
    {
        printf("[+] Found %s (pid %lu)\n", options.process.c_str(), pid);

        HANDLE process = OpenProcess(PROCESS_CREATE_THREAD |
            PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
            PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
        if (!process)
        {
            printf("[-] OpenProcess failed: %s\n",
                FormatError(GetLastError()).c_str());
            printf("[-] If the game is running elevated, run this injector as "
                "administrator too.\n");
            return false;
        }

        bool injected = false;

        do
        {
            if (!IsTargetCompatible(process))
            {
                printf("[-] %s is not a 32-bit process. This injector only "
                    "works with the 32-bit game.\n", options.process.c_str());
                break;
            }

            if (IsModuleLoaded(pid, DEFAULT_DLL))
            {
                printf("[*] %s is already loaded, nothing to do.\n", DEFAULT_DLL);
                injected = true;
                break;
            }

            // Wait until the game is pumping messages before touching it, so we
            // are not fighting the loader lock during start-up.
            printf("[*] Waiting for the game to finish starting up ...\n");
            WaitForInputIdle(process, 60000);

            if (options.delayMs)
            {
                printf("[*] Settling for %lu ms ...\n", options.delayMs);
                Sleep(options.delayMs);
            }

            // The game can die during a long settle (bad launch, alt-F4).
            DWORD exitCode = 0;
            if (GetExitCodeProcess(process, &exitCode) && exitCode != STILL_ACTIVE)
            {
                printf("[-] The game exited before it could be injected.\n");
                break;
            }

            printf("[*] Injecting %s ...\n", dllPath.c_str());
            injected = Inject(process, dllPath);
        } while (false);

        CloseHandle(process);
        return injected;
    }

    void WaitForExit(DWORD pid)
    {
        HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (!process)
            return;

        WaitForSingleObject(process, INFINITE);
        CloseHandle(process);
    }
}

int main(int argc, char **argv)
{
    Options options;
    if (!ParseArgs(argc, argv, options))
    {
        PrintUsage();
        return 1;
    }

    const std::string dllPath = LocateDll(options.dll);
    if (!FileExists(dllPath))
    {
        printf("[-] Cannot find the DLL: %s\n", dllPath.c_str());
        printf("[-] Build the solution first, or pass --dll <path>.\n");
        return 1;
    }

    printf("[*] DLL:    %s\n", dllPath.c_str());
    printf("[*] Target: %s\n", options.process.c_str());

    if (!EnableDebugPrivilege())
        printf("[*] Note: could not enable SeDebugPrivilege. That is usually "
            "fine, but run as administrator if injection fails.\n");

    if (!options.launch.empty() && !LaunchGame(options.launch))
        return 1;

    int result = 0;

    for (;;)
    {
        DWORD pid = WaitForGame(options);
        if (!pid)
        {
            printf("[-] Timed out waiting for %s.\n", options.process.c_str());
            return 1;
        }

        if (InjectIntoGame(pid, options, dllPath))
        {
            printf("[+] Done. Press Insert in-game to open the menu.\n");
        }
        else
        {
            if (!options.watch)
                return 1;
            result = 1;
        }

        if (!options.watch)
            break;

        printf("[*] Watching for the next launch. Close this window to stop.\n");
        WaitForExit(pid);

        // Do not immediately re-detect the process we just saw exit.
        Sleep(1000);
    }

    return result;
}
