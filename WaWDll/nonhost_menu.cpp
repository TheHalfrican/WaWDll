#include "stdafx.hpp"
// stdafx only pulls these in under _DEBUG, and settings persistence needs them
// in any configuration
#include <fstream>
#include <string>
#include "nonhost_menu.hpp"

namespace GameData
{
    int *cl_connectionState   = (int *)0x305842C;
    UiContext *uiDC           = (UiContext *)0x208E920;
    ScreenPlacement *scrPlace = (ScreenPlacement *)0x957360;
    KeyState *keys            = (KeyState *)0x951C44;
    HWND *hwnd                = (HWND *)0x22C1BE4;

    int (__stdcall *MessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
        = *(int (__stdcall **)(HWND, LPCSTR, LPCSTR, UINT))MessageBoxA_a;
    DWORD (__stdcall *timeGetTime)() = *(DWORD (__stdcall **)())timeGetTime_a;
    void (__stdcall *InitializeCriticalSection)(LPCRITICAL_SECTION lpCriticalSection)
        = *(void (__stdcall **)(LPCRITICAL_SECTION))InitializeCriticalSection_a;
    void (__stdcall *EnterCriticalSection)(LPCRITICAL_SECTION lpCriticalSection)
        = *(void (__stdcall **)(LPCRITICAL_SECTION))EnterCriticalSection_a;
    void (__stdcall *LeaveCriticalSection)(LPCRITICAL_SECTION lpCriticalSection)
        = *(void (__stdcall **)(LPCRITICAL_SECTION))LeaveCriticalSection_a;
    dvar_s *(__cdecl *Dvar_FindVar)(const char *dvarName)
        = (dvar_s *(__cdecl *)(const char *))Dvar_FindVar_a;
    Font_s *(__cdecl *R_RegisterFont)(const char *font, int imageTrac)
        = (Font_s *(__cdecl *)(const char *, int))R_RegisterFont_a;
    void *(__cdecl *Material_RegisterHandle)(const char *materialName, int imageTrac)
        = (void *(__cdecl *)(const char *, int))Material_RegisterHandle_a;
    void (__cdecl *CG_DrawRotatedPic)(ScreenPlacement *scrPlace, float x, float y,
        float width, float height, float angle, const float *color, void *material)
        = (void (__cdecl *)(ScreenPlacement *, float, float,
            float, float, float, const float*, void*))CG_DrawRotatedPic_a;
    void (__cdecl *R_AddCmdDrawStretchPicInternal)(const char *text, int maxChars,
        void *font, float x, float y, float xScale, float yScale, float rotation, int style)
        = (void (__cdecl *)(const char *, int, void *, float, float, 
            float, float, float, int))CL_DrawTextPhysical_a;
    int (__cdecl *UI_TextWidthInternal)(const char *text, int maxChars,
        void *font, float scale)
        = (int (__cdecl *)(const char *, int, void *, float))UI_TextWidth_a;
    char *(__cdecl *va)(const char *fmt, ...) 
        = (char *(__cdecl *)(const char *, ...))va_a;
    void (__cdecl *CG_GameMessage)(int localClientNum, const char *msg, int length)
        = (void (__cdecl *)(int, const char *, int))CG_GameMessage_a;
    void (__cdecl *Com_Error)(int code, const char *fmt, ...)
        = (void (__cdecl *)(int, const char *, ...))Com_Error_a;

    void __usercall Cbuf_AddText(const char *cmd)
    {
        DWORD addr = Cbuf_AddText_a;
        __asm
        {
            mov         ecx, 0
            mov         eax, cmd
            call        addr
        }
    }

    unsigned int Sys_Milliseconds()
    {
        return GameData::timeGetTime() - *(int *)0x22BEC34;
    }
    
    bool IN_IsForegroundWindow()
    {
        return *(bool *)(0x229A0D4);
    }

    Font_s *__usercall UI_GetFontHandle(ScreenPlacement *scrPlace, int fontEnum)
    {
        DWORD addr = UI_GetFontHandle_a;
        Font_s *result;
        __asm
        {
            mov         ecx, scrPlace
            mov         eax, fontEnum
            fldz
            sub         esp, 4
            fstp        [esp]
            call        addr
            add         esp, 4
            mov         result, eax
        }
        return result;
    }

    float __usercall UI_TextWidth(const char *text, int maxChars,
        Font_s *font, float scale)
    {
        float result;
        __asm
        {
            push        maxChars
            push        text
            mov         eax, font
            movss       xmm0, scale
            call        UI_TextWidthInternal
            add         esp, 8
            cvtsi2ss    xmm0, eax
            movss       result, xmm0
        }
        return result;
    }

    float UI_TextHeight(Font_s *font, float scale)
    {
        return static_cast<float>(
            floor(font->pixelHeight * R_NormalizedTextScale(font, scale) + 0.5));
    }

    void __usercall UI_DrawRect(ScreenPlacement *scrPlace, float x, float y, float width,
        float height, int horzAlign, int vertAlign, float thickness, const float *color)
    {
        DWORD addr = UI_DrawRect_a;
        __asm
        {
            mov         eax, color
            mov         ecx, horzAlign
            sub         esp, 4
            fld         thickness
            fstp        dword ptr [esp]
            push        vertAlign
            sub         esp, 10h
            fld         height
            fstp        dword ptr [esp + 0Ch]
            fld         width
            fstp        dword ptr [esp + 8]
            fld         y
            fstp        dword ptr [esp + 4]
            fld         x
            fstp        dword ptr [esp]
            push        scrPlace
            call        addr
            add         esp, 1Ch
        }
    }

    void __usercall UI_FillRect(ScreenPlacement *scrPlace, float x, float y, float width,
        float height, int horzAlign, int vertAlign, const float *color)
    {
        DWORD addr = UI_FillRect_a;
        __asm
        {
            sub         esp, 1Ch
            mov         ecx, scrPlace
            mov         edx, color
            mov         dword ptr [esp + 18h], edx
            mov         edx, vertAlign
            mov         dword ptr [esp + 14h], edx
            mov         edx, horzAlign
            mov         dword ptr [esp + 10h], edx
            fld         height
            fstp        dword ptr [esp + 0Ch]
            fld         width
            fstp        dword ptr [esp + 8]
            fld         y
            fstp        dword ptr [esp + 4]
            fld         x
            fstp        dword ptr [esp]
            call        addr
            add         esp, 1Ch
        }
    }

    void __usercall UI_DrawText(ScreenPlacement *scrPlace, const char *text,
        int maxChars, void *font, float x, float y, float scale,
        float angle, const float *color, int style)
    {
        DWORD addr = UI_DrawText_a;
        __asm
        {
            mov         eax, 0
            mov         ecx, 0
            push        style
            push        color
            push        scale
            push        y
            push        x
            push        font
            push        maxChars
            push        text
            push        scrPlace
            call        addr
            add         esp, 24h
        }
    }

    void __usercall R_AddCmdDrawStretchPic(const char *text, int maxChars,
        void *font, float x, float y, float xScale, float yScale,
        float rotation, const float *color, int style)
    {
        __asm
        {
            push        style
            sub         esp, 14h
            fld         rotation
            fstp        [esp + 10h]
            fld         yScale
            fstp        [esp + 0Ch]
            fld         xScale
            fstp        [esp + 08h]
            fld         y
            fstp        [esp + 04h]
            fld         x
            fstp        [esp]
            push        font
            push        maxChars
            push        text
            mov         ecx, color
            call        R_AddCmdDrawStretchPicInternal
            add         esp, 24h
        }
    }

    void __usercall ScrPlace_ApplyRect(ScreenPlacement *scrPlace,
        float *x, float *y, float *w, float *h, int horzAlign, int vertAlign)
    {
        DWORD addr = ScrPlace_ApplyRect_a;
        __asm
        {
            mov         edx, x
            mov         ecx, w
            mov         edi, y
            mov         esi, h
            push        vertAlign
            push        horzAlign
            push        scrPlace
            call        addr
            add         esp, 0Ch
        }
    }

    void __usercall CG_DrawRotatedPicPhysical(ScreenPlacement *scrPlace, float x, float y,
        float width, float height, float angle, const float *color, void *material)
    {
        DWORD addr = CG_DrawRotatedPicPhysical_a;
        __asm
        {
            push        material
            push        color
            sub         esp, 10h
            fld         x
            fstp        [esp]
            fld         y
            fstp        [esp + 4]
            fld         width
            fstp        [esp + 8]
            fld         height
            fstp        [esp + 0Ch]
            movss       xmm0, angle
            mov         edx, scrPlace
            call        addr
            add         esp, 18h
        }
    }

    int __usercall Key_StringToKeynum(const char *name)
    {
        DWORD result;
        DWORD addr = Key_StringToKeynum_a;
        __asm
        {
            mov         edi, name
            call        addr
            mov         result, eax
        }
        return result;
    }

    bool Key_IsDown(const char *bind)
    {
        for (__int16 i = 0; i < 256; i++)
            if (keys[i].binding)
                if (!strcmp(keys[i].binding, bind))
                    if (keys[i].down)
                        return true;
        return false;
    }

    float R_NormalizedTextScale(Font_s *font, float scale)
    {
        return scale * 48.0f / (float)font->pixelHeight;
    }

    float R_TextHeight(Font_s *font)
    {
        return static_cast<float>(font->pixelHeight);
    }

    float __usercall R_TextWidth(const char *text, int maxChars, Font_s *font)
    {
        DWORD addr = R_TextWidth_a;
        float result;
        __asm
        {
            mov         eax, text
            push        font
            push        maxChars
            call        addr
            add         esp, 8
            cvtsi2ss    xmm0, eax
            movss       result, xmm0
        }
        return result;
    }

    void __usercall DrawSketchPicGun(ScreenPlacement *scrPlace, rectDef_s *rect,
        const float *color, Material *material, int ratio)
    {
        DWORD addr = DrawSketchPicGun_a;
        __asm
        {
            mov         eax, rect
            push        ratio
            push        material
            push        color
            push        scrPlace
            call        addr
            add         esp, 10h
        }
    }

#ifdef _DEBUG
    LONG (__stdcall *TopLevelExceptionFilter)(struct _EXCEPTION_POINTERS *ExceptionInfo)
        = (LONG (__stdcall *)(_EXCEPTION_POINTERS *))TopLevelExceptionFilter_a;
    void TopLevelExceptionFilterDetour(struct _EXCEPTION_POINTERS *ExceptionInfo)
    {
        PDWORD_PTR currESP =
            (PDWORD_PTR)ExceptionInfo->ContextRecord->Esp;
        PDWORD_PTR currEIP =
            (PDWORD_PTR)ExceptionInfo->ContextRecord->Eip;

        // Print all the register values
        std::cerr << std::hex;
        std::cerr << "EAX: " << (void *)ExceptionInfo->ContextRecord->Eax << std::endl;
        std::cerr << "EBX: " << (void *)ExceptionInfo->ContextRecord->Ebx << std::endl;
        std::cerr << "ECX: " << (void *)ExceptionInfo->ContextRecord->Ecx << std::endl;
        std::cerr << "EDX: " << (void *)ExceptionInfo->ContextRecord->Edx << std::endl;
        std::cerr << "EDI: " << (void *)ExceptionInfo->ContextRecord->Edi << std::endl;
        std::cerr << "ESI: " << (void *)ExceptionInfo->ContextRecord->Esi << std::endl;
        std::cerr << "EBP: " << (void *)ExceptionInfo->ContextRecord->Ebp << std::endl;
        std::cerr << "ESP: " << (void *)currESP << std::endl;
        std::cerr << "EIP: " << (void *)currEIP << std::endl;

        // Specific for my code!
        // Finds all enum addresses stored in an anonymous enum to be used for matches
        // with the call stack values
        TCHAR szFolderPath[MAX_PATH];
        std::unordered_map<DWORD, std::string> addrs;
        // Look for my documents directory
        if (SHGetSpecialFolderPath(*hwnd, szFolderPath, CSIDL_MYDOCUMENTS, false))
        {
            // Append my directory for my project and Iterate through all files in project directory
            std::string path(szFolderPath);
            std::filesystem::directory_iterator it(path + "\\Visual Studio 2017\\WaWDll\\WaWDll");
            for (const auto &i : it)
            {
                // Look for the header files in the project
                std::string str = i.path().string();
                size_t index;
                if ((index = str.find(".hpp")) == std::string::npos)
                    continue;

                // Find where an anonymous enum is located and begin parsing
                std::ifstream file(str);
                if (file.good())
                {
                    std::string tmp;
                    bool startParsing = false;
                    while (std::getline(file, tmp))
                    {
                        if (tmp.find("enum") != std::string::npos)
                        {
                            startParsing = true;
                            break;
                        }
                    }

                    if (startParsing)
                    {
                        while (std::getline(file, tmp))
                        {
                            // Find the part with the function name and address, 
                            // remove the prefix and suffix from the message
                            // and store it in the map of addresses
                            int nameStart, addrStart;
                            if ((nameStart = tmp.find("_a")) != std::string::npos
                                && (addrStart = tmp.find("= 0x")) != std::string::npos)
                            {
                                std::string name = tmp.substr(0, nameStart);
                                std::string addr = tmp.substr(addrStart + 4);
                                while (isspace(name.at(0)))
                                    name.erase(0, 1);
                                addr.pop_back();
                                addrs.insert(
                                    std::pair<int, std::string>(strtol(addr.c_str(), nullptr, 0x10), name)
                                );
                            }
                            if (tmp.find("}") != std::string::npos)
                                break;
                        }
                    }
                }
                file.close();
            }
        }

        constexpr unsigned TEXTSEGSTART = 0x401000;
        constexpr unsigned TEXTSEGEND = 0x7EB000;
        // For 8 addresses, print the caller address 
        std::cerr << "\nCALL STACK:\n";
        for (int i = 0; i < 8; currESP++)
        {
            // If the value on the stack exists in the text segment
            if (*currESP >= TEXTSEGSTART && *currESP <= TEXTSEGEND)
            {
                // Get the return location and add the bytes the relative offset in the opcode
                DWORD caller = *(DWORD *)((int)*currESP - 4) + *currESP;
                if (caller >= TEXTSEGSTART && caller <= TEXTSEGEND)
                {
                    // See if the caller matches any functions defined in this project
                    // if not 
                    auto result = std::find_if(addrs.begin(), addrs.end(),
                        [caller](const auto &i)
                        {
                            return i.first == caller;
                        });
                    if (result != addrs.end())
                        std::cerr << result->second << std::endl;
                    else
                        std::cerr << (void *)caller << std::endl;
                    i++;
                }
            }
        }

        // Reset the stack pointer copy and print the next
        currESP = (PDWORD_PTR)ExceptionInfo->ContextRecord->Esp;
        std::cerr << "\nSTACK VIEW:\n";
        for (int i = 0; i < 8; i++)
        {
            if (i)
                ++currESP;
            std::cerr << (void *)currESP << ": " << (void *)*currESP << " ";
            for (int j = 0; j < 3; j++)
                std::cerr << " " << (void *)*(++currESP);
            std::cerr << std::endl;
        }

        if (CopyAddressToClipboard(currEIP))
            std::cerr << "\nInstruction pointer copied to clipboard\n";
        else
            std::cerr << "\nProblem copying instruction pointer to clipboard\n";
    }
#endif

    void __usercall *Menu_PaintAll = (void __usercall *)Menu_PaintAll_a;
    void __declspec(naked) Menu_PaintAllDetourInvoke(UiContext *dc)
    {
        __asm
        {
            push        esi
            call        Menu_PaintAllDetour
            add         esp, 4
            pop         edi
            pop         ebp
            pop         ebx
            add         esp, 410h
            ret
        }
    }
    // Keeps the debug console from taking focus off a fullscreen game.
    //
    // The console window belongs to the console host process, not to us, and is
    // created asynchronously, so minimising it in DllMain can run before the
    // window even exists and the host may activate it afterwards regardless.
    // This runs from the render thread to catch it whenever it turns up.
    //
    // Handing focus back with SetForegroundWindow was tried first, including
    // with AttachThreadInput to lift the foreground restriction, and did not
    // hold. Minimising does: ShowWindow on another process's window is not
    // restricted the way SetForegroundWindow is, and minimising the foreground
    // window makes Windows activate the one behind it, which is the game.
    void KeepConsoleMinimized()
    {
        // Long enough to outlast a slow console host, short enough that
        // restoring the console yourself is not fought for more than a moment
        static unsigned int deadline = Sys_Milliseconds() + 3000;
        static bool finished = false;

        if (finished)
            return;

        if (Sys_Milliseconds() > deadline)
        {
            finished = true;
            return;
        }

        HWND console = GetConsoleWindow();
        if (!console || IsIconic(console))
            return;

        ShowWindow(console, SW_MINIMIZE);

        // Minimising normally hands the foreground back by itself, but ask
        // directly in case something else ended up in front
        if (hwnd && *hwnd)
            SetForegroundWindow(*hwnd);
    }

    void Menu_PaintAllDetour(UiContext *dc)
    {
        Menu &menu = Menu::Instance();
       // EnterCriticalSection(&menu.critSection);

        KeepConsoleMinimized();
        menu.EnforceDvars();

        if (IN_IsForegroundWindow())
            menu.MonitorKeys();

        if (menu.open)
            menu.Execute();

        // Gated on the menu option. This used to run unconditionally, so boxes
        // were drawn from the moment the DLL was injected and the Enemy ESP
        // toggle did nothing but flip its own checkbox.
        if (menu.GetOptionData(ESP_MENU, "Enemy ESP").data.boolean)
            RenderESP();

        /*
            To call a GSC method, you must reverse yyparse() in ScriptParse()
            as this function fills the sval_u nodes which consitiute the arguments.
            In VM_Execute(), the game uses the gScrCompilePub[inst].func_table to 
            get the functions to call. This table is populated during EmitCall during
            the call to ScriptCompile().

            TODO figure out how to access all the GSC functions using in VM_Execute()
            Steps I currently know:
            1. It is converted func_name from sval_u into a string using SL_ConvertToString
            2. it uses GetFunction() to get the function pointer
            3. It uses AddFunction to add it to gScrCompilePub[inst].func_table 
            4. The function is called somehow in VM_Execute. Don't know yet
        */
       
       // LeaveCriticalSection(&menu.critSection);
    }
    
    int (__cdecl *Menu_HandleMouseMove)(ScreenPlacement *scrPlace, void *menu)
        = (int (__cdecl *)(ScreenPlacement *, void *))Menu_HandleMouseMove_a;
    int Menu_HandleMouseMoveDetour(ScreenPlacement *scrPlace, void *item)
    {
        Menu &menu = Menu::Instance();
        //GameData::EnterCriticalSection(&menu.critSection);

        if (!menu.open)
        { 
           // GameData::LeaveCriticalSection(&menu.critSection);
            return GameData::Menu_HandleMouseMove(scrPlace, item);
        }
    
        //GameData::LeaveCriticalSection(&menu.critSection);
        return 0;
    }

    void (__cdecl *CL_KeyEvent)(int localClientNum, int value, int down,
        unsigned int time) = (void(__cdecl *)(int, int, int, unsigned int))CL_KeyEvent_a;
    void CL_KeyEventDetour(int localClientNum, int key, int down, int time)
    {
        Menu &menu = Menu::Instance();
        GameData::EnterCriticalSection(&menu.critSection);

        OptionData &aimKey = menu.GetOptionData(AIMBOT_MENU, "Aim Key");
        OptionData &autoShoot = menu.GetOptionData(AIMBOT_MENU, "Auto Shoot");

        if (InGame() && GameData::keys[key].binding
            && !*(int *)0x208E938
            && (aimKey.data.integer != 1 || !strcmp(GameData::keys[key].binding, "+attack"))
            && autoShoot.data.boolean)
            return;

        GameData::LeaveCriticalSection(&menu.critSection);
        return GameData::CL_KeyEvent(localClientNum, key, down, time);
    }

    void __usercall *Cbuf_AddTextHook = (void __usercall *)Cbuf_AddText_a;
    void __declspec(naked) Cbuf_AddTextDetourInvoke(const char *text,
        int localClientNum)
    {
        __asm
        {
            push        eax
            push        ecx
            push        ecx
            push        eax
            call        Cbuf_AddTextDetour
            add         esp, 8h
            cmp         al, 0
            pop         ecx
            pop         eax
            jz          LABEL_1
            push        ebp
            push        esi
            push        edi
            push        22990F8h
            push        594208h
            ret
    LABEL_1:
            pop         edx
            jmp         edx
        }
    }
    bool Cbuf_AddTextDetour(const char *text, int localClientNum)
    {
        return true;
    }

    void (__cdecl *IN_MouseEvent)(int mstate) = (void (__cdecl *)(int))IN_MouseEvent_a;
    void IN_MouseEventDetour(int mstate)
    {
        Menu &menu = Menu::Instance();
        //GameData::EnterCriticalSection(&menu.critSection);

        if (!menu.open)
        {
            GameData::LeaveCriticalSection(&menu.critSection);
            return GameData::IN_MouseEvent(mstate);
        }

       // GameData::LeaveCriticalSection(&menu.critSection);
    }

    int (__cdecl *Com_Printf)(int channel, const char *format, ...)
        = (int (__cdecl *)(int, const char *, ...))Com_Printf_a;
    int Com_PrintfDetour(int channel, const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);

        char printBuffer[1024];
        vsnprintf(printBuffer, 1024, format, ap);
        printf("%s\n", printBuffer);

        va_end(ap);
        return 0;
    }
}

std::unordered_map<std::string, GameData::dvar_s *> dvars;

Fonts::Font Fonts::normalFont = { 1, "fonts/normalFont" };

Colors::Color Colors::white            = { 255.0f, 255.0f, 255.0f, 255.0f };
Colors::Color Colors::black            = {   0.0f,   0.0f,   0.0f, 255.0f };
Colors::Color Colors::red              = { 255.0f,   0.0f,   0.0f, 255.0f };
Colors::Color Colors::green            = {   0.0f, 255.0f,   0.0f, 255.0f };
Colors::Color Colors::blue             = {   0.0f,   0.0f, 255.0f, 255.0f };
Colors::Color Colors::transparentBlack = {   0.0f,   0.0f,   0.0f, 100.0f };

// A menu option driving a dvar that the menu cannot write once and forget.
// Two problems share one answer:
//
//   - These dvars belong to the server game module, which does not register
//     them until a map has loaded. They cannot go in the InsertDvar startup
//     chain, because the menu is built lazily on the first paint, at the main
//     menu, and resolving them there would hit the Com_Error.
//   - They are cheat protected, so the engine resets them to stock on map load
//     unless sv_cheats is set.
//
// Both are answered by resolving on demand and re-asserting from the render
// thread, which is what the BO3Z trainer converged on for run speed after a
// one shot write left its menu showing a value the game was not using.
//
// isFloat is the field that earns this struct. DvarValue is a union, and which
// member the engine reads is not guessable and is NOT consistent between these
// three: g_speed is an int while jump_height and g_gravity are floats, all
// confirmed by reading the live process with Tools/dvar_probe.py. Writing the
// wrong member stores a denormal, so an int 39 into jump_height is 5.5e-44 and
// the player simply cannot jump.
struct EnforcedDvar
{
    const char       *dvar;
    const char       *option;
    // The value the engine resets to, which is also the option's off position.
    // It is whichever end of the range means stock, so for gravity it is the
    // maximum rather than the minimum.
    int               stock;
    int               min;
    int               max;
    int               step;
    bool              isFloat;
    GameData::dvar_s *cached;
};

// Steps are sized so spinning a range end to end is a second or two of
// holding the mouse. An earlier pass used fine steps over narrow ranges, and
// the extremes were then 20 to 30 clicks away, so in practice nobody reached
// them and both options felt like they did nothing.
//
// The ranges are deliberately far past sensible. Jump Height tops out at
// roughly 20 times stock, and Gravity bottoms out at a sixteenth of it.
//
// Gravity descends from its stock 800 because lower is floatier. It is not
// independent of Jump Height, it multiplies it: observed in game, the lower
// the gravity the higher the jump as well as the longer the hang time, and at
// stock gravity raising Jump Height alone does much less than expected. The
// two are meant to be tuned together, and the useful part of the gravity range
// is right down at the bottom.
//
// Resist the tidy sqrt(2 * gravity * jump_height) reading of the launch
// velocity, which predicts an apex depending on jump_height alone. That is not
// what the game does. The interaction above is observed; the mechanism behind
// it has not been traced through the movement code, so do not write it down as
// though it had been.
static EnforcedDvar enforcedDvars[] =
{
    { "g_speed",     "Move Speed",  190, 190, 600, 10, false, nullptr },
    { "jump_height", "Jump Height",  39,  39, 839, 50, true,  nullptr },
    { "g_gravity",   "Gravity",     800,  50, 800, 50, true,  nullptr },
};

static EnforcedDvar *FindEnforcedDvar(const char *option)
{
    for (EnforcedDvar &enforced : enforcedDvars)
        if (!strcmp(enforced.option, option))
            return &enforced;

    return nullptr;
}

// Cached because the dvar pool is static: once registered, the pointer holds
// for the rest of the process.
static GameData::dvar_s *ResolveEnforcedDvar(EnforcedDvar &enforced)
{
    if (!enforced.cached)
    {
        auto entry = dvars.find(enforced.dvar);
        if (entry != dvars.end())
            enforced.cached = entry->second;
        else if (InsertDvar(enforced.dvar))
            enforced.cached = dvars.at(enforced.dvar);
    }

    return enforced.cached;
}

// Only writes when the live value differs from the wanted one. Fewer writes
// means fewer chances to land in the middle of a state transition, which is
// the same reasoning behind BO3Z's WriteRunSpeedIfChanged.
static void WriteEnforcedDvar(EnforcedDvar &enforced, int value)
{
    GameData::dvar_s *dvar = ResolveEnforcedDvar(enforced);
    if (!dvar)
        return;

    if (enforced.isFloat)
    {
        float wanted = static_cast<float>(value);
        if (dvar->current.value != wanted)
            dvar->current.value = wanted;
    }
    else if (dvar->current.integer != value)
        dvar->current.integer = value;
}

OptionData::OptionData(OptionType type) : type(type)
{
    switch (type)
    {
        case TYPE_INT:
            this->data = Data(0);
            break;
        case TYPE_BOOL:
        case TYPE_TOGGLE:
            this->data = Data(false);
            break;
        case TYPE_FLOAT:
            this->data = Data(0.0f);
            break;
        default:
            this->data = Data(0);
            break;
    }
}

Menu::Menu() :
    open(false),
    toggled(false),
    currentSub(MAIN_MENU),
    timer(0),
    toggleKeyWasDown(false),
    leftWasDown(false),
    rightWasDown(false),
    leftPressed(false),
    rightPressed(false)
{
    GameData::InitializeCriticalSection(&this->critSection);


    for (int sub = MAIN_MENU; sub <= HUD_MENU; sub++)
        this->options.push_back(std::unordered_map<std::string, Option>());

    // Main menu options
    Insert(MAIN_MENU, "Aimbot Menu", TYPE_SUB, 
        []() 
        { 
            Menu::Instance().LoadSub(AIMBOT_MENU); 
        });
    Insert(MAIN_MENU, "ESP Menu", TYPE_SUB, 
        []() 
        { 
            Menu::Instance().LoadSub(ESP_MENU); 
        });
    Insert(MAIN_MENU, "HUD Menu", TYPE_SUB, 
        []() 
        { 
            Menu::Instance().LoadSub(HUD_MENU); 
        });
    Insert(MAIN_MENU, "Misc Menu", TYPE_SUB, 
        []() 
        { 
            Menu::Instance().LoadSub(MISC_MENU); 
        });

    // ESP menu options
    Insert(ESP_MENU, "Enemy ESP", TYPE_BOOL, 
        []() 
        { 
            Menu::Instance().BoolModify("Enemy ESP"); 
        });
    Insert(ESP_MENU, "Friendly ESP", TYPE_BOOL, 
        []() 
        { 
            Menu::Instance().BoolModify("Friendly ESP"); 
        });

    // HUD menu options
    Insert(HUD_MENU, "Server Info", TYPE_BOOL, 
        []() 
        { 
            Menu::Instance().BoolModify("Server Info"); 
        });

    // Aimbot menu options
    Insert(AIMBOT_MENU, "Enable Aimbot", TYPE_BOOL, 
        []() 
        { 
            Menu::Instance().BoolModify("Enable Aimbot"); 
        });
    Insert(AIMBOT_MENU, "Aim Key",  TYPE_INT, 
        []() 
        { 
            Menu::Instance().IntModify("Aim Key", TYPE_INT, 0, 2); 
        });
    Insert(AIMBOT_MENU, "Auto Shoot", TYPE_BOOL, 
        []() 
        { 
            Menu::Instance().BoolModify("Auto Shoot"); 
        });
    Insert(AIMBOT_MENU, "No Recoil", TYPE_BOOL, 
        []() 
        {
            WriteBytes(0x46A87E,
                Menu::Instance().BoolModify("No Recoil") ? "\xEB" : "\x74", 1);
        });
    Insert(AIMBOT_MENU, "No Spread", TYPE_BOOL, 
        []() 
        {
            Menu::Instance().BoolModify("No Spread"); 
        });

    // Miscellaneous menu options
    Insert(MISC_MENU, "FOV",  TYPE_INT, 
        []() 
        {
            dvars.at("cg_fov")->current.value 
                = static_cast<float>(Menu::Instance().IntModify("FOV", TYPE_INT, 65, 125));
        });
    // The three below all drive a dvar through the enforcedDvars table, which
    // owns their ranges and their stock values. Each stock doubles as the
    // option's off position, so none of them has a separate boolean that could
    // fall out of sync with the number. They are written straight into the
    // dvar rather than through Cbuf_AddText, because all three are cheat
    // protected and a direct write skips that, the same way FOV above does.
    //
    // g_speed is the engine's base movement speed in units per second, and
    // walk, sprint and crouch all scale off it, so it covers the lot.
    Insert(MISC_MENU, "Move Speed", TYPE_INT,
        []()
        {
            Menu::Instance().EnforcedDvarModify("Move Speed");
        });
    Insert(MISC_MENU, "Jump Height", TYPE_INT,
        []()
        {
            Menu::Instance().EnforcedDvarModify("Jump Height");
        });
    // Lower is floatier. Worth knowing that low gravity plus a high jump makes
    // it easy to reach geometry the map does not expect, and in Zombies being
    // stranded where the zombies cannot path to you soft locks the round.
    Insert(MISC_MENU, "Gravity", TYPE_INT,
        []()
        {
            Menu::Instance().EnforcedDvarModify("Gravity");
        });
    Insert(MISC_MENU, "Super Steady Aim", TYPE_BOOL, 
        []() 
        { 
            WriteBytes(0x41DB2B,
                Menu::Instance().BoolModify("Super Steady Aim")
                ? "\x90\x90\x90\x90\x90"
                : "\x83\xFF\x02\x75\x15", 5);
        });
    Insert(MISC_MENU, "Enable Cheats", TYPE_BOOL, 
        []() 
        { 
            dvars.at("sv_cheats")->current.enabled 
                = Menu::Instance().BoolModify("Enable Cheats");
        });
    // The three below are toggles in the game, so they track what has been
    // clicked and show it as On/Off. Give All Weapons is a one shot action with
    // no state, so it stays a plain command with no indicator.
    Insert(MISC_MENU, "God Mode", TYPE_TOGGLE,
        []()
        {
            Menu::Instance().BoolModify("God Mode");
            GameData::Cbuf_AddText("god");
        });
    Insert(MISC_MENU, "No Clip", TYPE_TOGGLE,
        []()
        {
            Menu::Instance().BoolModify("No Clip");
            GameData::Cbuf_AddText("noclip");
        });
    Insert(MISC_MENU, "Give All Weapons", TYPE_VOID,
        []()
        {
            GameData::Cbuf_AddText("give all");
        });
    Insert(MISC_MENU, "No Target", TYPE_TOGGLE,
        []()
        {
            Menu::Instance().BoolModify("No Target");
            GameData::Cbuf_AddText("notarget");
        });
    Insert(MISC_MENU, "Infinite Ammo", TYPE_BOOL, 
        []() 
        {
            dvars.at("player_sustainAmmo")->current.enabled 
                = Menu::Instance().BoolModify("Infinite Ammo");
        });
    Insert(MISC_MENU, "No Flinch", TYPE_BOOL, 
        []() 
        { 
            Menu::Instance().BoolModify("No Flinch"); 
        });

    if (InsertDvar("cl_ingame")
        && InsertDvar("cg_fov")
        && InsertDvar("perk_weapSpreadMultiplier")
        && InsertDvar("sv_cheats")
        && InsertDvar("player_sustainAmmo"))
    {
        // Defaults first, so a saved file overrides them and a missing one
        // still leaves every option somewhere sensible
        this->GetOptionData(MISC_MENU, "FOV").data.integer = 65;
        for (const EnforcedDvar &enforced : enforcedDvars)
            this->GetOptionData(MISC_MENU, enforced.option).data.integer
                = enforced.stock;
        this->GetOptionData(AIMBOT_MENU, "Aim Key").data.integer = 2;

        this->LoadSettings();
        this->ApplySettings();
    }
    else
        GameData::Com_Error(0, "Dvars failed to load\n");
}

void Menu::Insert(int sub, const char *option, OptionType type, 
    std::function<void()> &&callback)
{
    this->options.at(sub).insert(
        std::pair<std::string, Option>(
            option, Option(option, type, std::forward<std::function<void()>>(callback))
            )
    );
}

void Menu::LoadSub(Submenu sub)
{
    this->currentSub = sub;
}

void Menu::CloseSub()
{
    switch (this->currentSub)
    {
        case MAIN_MENU:
            this->open = false;
            break;
        case AIMBOT_MENU:
        case ESP_MENU:
        case HUD_MENU:
        case MISC_MENU:
            this->currentSub = MAIN_MENU;
            break;
        default:
            this->open = false;
            break;
    }

    // Backing out of the last menu is one of the two ways the menu closes
    if (!this->open)
        this->SaveSettings();
}

// Settings live beside the DLL so they travel with it. __ImageBase is this
// module's base address, which avoids having to thread the HMODULE from DllMain
// down to here.
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

static std::string SettingsPath()
{
    char path[MAX_PATH] = { 0 };
    DWORD length = GetModuleFileNameA((HMODULE)&__ImageBase, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
        return std::string();

    std::string full(path, length);
    size_t slash = full.find_last_of("\\/");
    if (slash == std::string::npos)
        return std::string();

    return full.substr(0, slash + 1) + "WaWDll.cfg";
}

void Menu::SaveSettings()
{
    const std::string path = SettingsPath();
    if (path.empty())
        return;

    std::ofstream file(path, std::ios::trunc);
    if (!file)
        return;

    file << "# WaWDll settings. Rewritten whenever the menu closes.\n";
    file << "# Format: submenu|option=value. Delete this file to reset.\n";

    for (size_t sub = 0; sub < this->options.size(); sub++)
    {
        for (const auto &entry : this->options[sub])
        {
            const OptionData &var = entry.second.var;

            // Submenu links and one shot commands carry no state worth keeping
            if (var.type != TYPE_BOOL && var.type != TYPE_INT)
                continue;

            file << sub << '|' << entry.first << '='
                << (var.type == TYPE_BOOL
                    ? (var.data.boolean ? 1 : 0) : var.data.integer)
                << '\n';
        }
    }
}

void Menu::LoadSettings()
{
    const std::string path = SettingsPath();
    if (path.empty())
        return;

    std::ifstream file(path);
    if (!file)
        return;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        size_t bar = line.find('|');
        size_t equals = line.rfind('=');
        if (bar == std::string::npos || equals == std::string::npos || equals < bar)
            continue;

        int sub = atoi(line.substr(0, bar).c_str());
        if (sub < 0 || sub >= static_cast<int>(this->options.size()))
            continue;

        // Options that no longer exist are ignored rather than treated as an
        // error, so removing one does not invalidate everybody's saved file
        std::string name = line.substr(bar + 1, equals - bar - 1);
        auto entry = this->options[sub].find(name);
        if (entry == this->options[sub].end())
            continue;

        int value = atoi(line.substr(equals + 1).c_str());
        OptionData &var = entry->second.var;

        if (var.type == TYPE_BOOL)
            var.data.boolean = value != 0;
        else if (var.type == TYPE_INT)
            var.data.integer = value;
    }
}

void Menu::ApplySettings()
{
    dvars.at("cg_fov")->current.value =
        static_cast<float>(this->GetOptionData(MISC_MENU, "FOV").data.integer);
    dvars.at("sv_cheats")->current.enabled =
        this->GetOptionData(MISC_MENU, "Enable Cheats").data.boolean;
    dvars.at("player_sustainAmmo")->current.enabled =
        this->GetOptionData(MISC_MENU, "Infinite Ammo").data.boolean;
    this->EnforceDvars();

    WriteBytes(0x41DB2B,
        this->GetOptionData(MISC_MENU, "Super Steady Aim").data.boolean
        ? "\x90\x90\x90\x90\x90" : "\x83\xFF\x02\x75\x15", 5);
    WriteBytes(0x46A87E,
        this->GetOptionData(AIMBOT_MENU, "No Recoil").data.boolean
        ? "\xEB" : "\x74", 1);
}

// Spins the option's number and pushes it in immediately, so a click is felt
// at once rather than waiting on the next enforcement pass.
void Menu::EnforcedDvarModify(const char *option)
{
    EnforcedDvar *enforced = FindEnforcedDvar(option);
    if (!enforced)
        return;

    WriteEnforcedDvar(*enforced, this->IntModify(option, TYPE_INT,
        enforced->min, enforced->max, enforced->step));
}

// Re-asserts every enforced dvar, because the engine resets cheat protected
// dvars on map load and none of these exist before the first map loads.
//
// Called every frame, so it is gated: an option left at stock costs one
// integer compare, and never resolves a dvar, writes, or touches the game.
// Only a value the user actually asked for does any work, and even then only
// when the live value has drifted from it.
void Menu::EnforceDvars()
{
    for (EnforcedDvar &enforced : enforcedDvars)
    {
        int wanted = this->GetOptionData(MISC_MENU, enforced.option).data.integer;
        if (wanted == enforced.stock)
            continue;

        WriteEnforcedDvar(enforced, wanted);
    }
}

bool Menu::BoolModify(const std::string &varName)
{
    OptionData &var = this->GetOptionData(this->currentSub, varName);
    return var.data.boolean = !var.data.boolean;
}

int Menu::IntModify(const std::string &varName, OptionType type, int min, int max,
    int step)
{
    OptionData &var = this->GetOptionData(this->currentSub, varName);

    if (this->toggled)
        var.data.integer += step;
    else
        var.data.integer -= step;

    if (var.data.integer > max)
        var.data.integer = min;
    if (var.data.integer < min)
        var.data.integer = max;

    return var.data.integer;
}

OptionData &Menu::GetOptionData(Submenu sub, const std::string &key)
{
    return this->options.at(sub).at(key).var;
}

bool Menu::Ready()
{
    return GameData::Sys_Milliseconds() > this->timer;
}

void Menu::Wait(int ms)
{
    this->timer = GameData::Sys_Milliseconds() + ms;
}

void Menu::Execute()
{
    const char *title = "WaW Zombies DLL By E7ite";
    float menuCenterX = GameData::uiDC->screenDimensions[0] / 2
        / GameData::scrPlace->scaleVirtualToFull[0];
    float menuCenterY = GameData::uiDC->screenDimensions[1] / 2
        / GameData::scrPlace->scaleVirtualToFull[1];

    // Get x position of text aligned with a background and scaled for all resolutions
    float textWidth, textHeight;
    GameData::Font_s *fontPointer;
    float xAligned = AlignText(title, Fonts::normalFont, 0.3f,
        menuCenterX, ALIGN_CENTER, 1, 1, &fontPointer, &textWidth, &textHeight);

    // Get position and dimensions of all border and options
    float borderW = menuCenterX - 20;
    float borderH = textHeight * options[currentSub].size();
    float borderX = menuCenterX - borderW / 2;
    float borderY = menuCenterY - 98;
    float optionX = borderX + 4;
    float optionY = menuCenterY - 100;
    float optionH = UI_TextHeight(fontPointer, 0.3f);

    // Draw the title and the menu base
    optionY += RenderUITextWithBackground(title, xAligned, optionY, textWidth, 
        textHeight, Colors::blue, Colors::white, fontPointer, 0.3f);
    GameData::UI_FillRect(GameData::scrPlace, borderX, borderY, borderW, borderH, 0, 0,
        Colors::transparentBlack);
    GameData::UI_DrawRect(GameData::scrPlace, borderX, borderY, borderW, borderH,
        0, 0, 2, Colors::blue);

    // Draw all the options in the current sub menu
    for (auto &i : options[currentSub])
    {
        Colors::Color color = Colors::white;
        const Option &option = i.second;

        // Adjust options in menu based on mouse position and execute any callbacks
        if (this->MonitorMouse(i.second, borderX, optionY - 2, borderW, optionH + 2))
            color = Colors::blue;

        // Draw the additional visuals for array and boolean options
        switch (option.var.type)
        {
            case TYPE_BOOL:
                GameData::UI_FillRect(GameData::scrPlace, borderX + borderW - 12,
                    optionY - optionH + 2, 8, 8, 0, 0,
                    option.var.data.boolean ? Colors::blue : Colors::transparentBlack);
                break;
            case TYPE_INT:
            {
                std::string data = std::to_string(option.var.data.integer);
                RenderUIText(data.data(),
                    AlignText(data.data(), Fonts::normalFont,
                        0.3f, borderX + borderW - 3, ALIGN_RIGHT, 1, 0),
                    optionY, 0.3f, color, fontPointer);
                break;
            }
            case TYPE_TOGGLE:
            {
                // Reflects clicks made here, not the game's own state, which it
                // never reports back. Text rather than a checkbox so it reads
                // differently from the booleans the menu actually owns.
                char state[4];
                strcpy_s(state, option.var.data.boolean ? "On" : "Off");
                RenderUIText(state,
                    AlignText(state, Fonts::normalFont,
                        0.3f, borderX + borderW - 3, ALIGN_RIGHT, 1, 0),
                    optionY, 0.3f, color, fontPointer);
                break;
            }
        }

        // Draw the text of the menu
        optionY += RenderUIText(i.first, optionX, optionY,
            0.3f, color, fontPointer);
    }
}

bool Menu::MonitorMouse(Option &opt, float optionX, float optionY,
    float optionW, float optionH)
{
    if (GameData::uiDC->cursorPos[0] > optionX
        && GameData::uiDC->cursorPos[0] < optionX + optionW
        && GameData::uiDC->cursorPos[1] > optionY - optionH
        && GameData::uiDC->cursorPos[1] < optionY)
    {
        if (this->Ready())
        {
            // Numeric options keep hold to repeat, since spinning a value from
            // one end of its range to the other is the point and repeating one
            // is harmless. Everything else fires once per press: those callbacks
            // toggle god mode, run console commands and patch game code, and
            // reacting to the button being held meant a click that lingered or
            // drifted a few pixels ran every option it passed over.
            bool repeat = opt.var.type == TYPE_INT;
            int delay = repeat ? 100 : 200;

            bool left = repeat
                ? (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 : this->leftPressed;
            bool right = repeat
                ? (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0 : this->rightPressed;

            if (left)
            {
                this->toggled = true;
                opt.callback();
                this->toggled = false;
                this->Wait(delay);
            }
            else if (right)
            {
                opt.callback();
                this->Wait(delay);
            }

            // Persist single click changes straight away so they survive the
            // game being killed with the menu still open. Numeric options are
            // left to the save on close, since writing the file on every step
            // of a hold to repeat would mean a disk write every 100ms from the
            // render thread.
            if ((left || right) && !repeat)
                this->SaveSettings();
        }
        return true;
    }
    return false;
}

void Menu::MonitorKeys()
{
    // Edge detection lives outside the Ready() gate on purpose. Ready() can be
    // false for a couple of hundred milliseconds after any input, and if the
    // key state were only sampled inside it, a press and release landing in
    // that window would be missed entirely.
    bool toggleDown = (GetAsyncKeyState(MENU_TOGGLE_KEY) & 0x8000) != 0;
    bool togglePressed = toggleDown && !this->toggleKeyWasDown;
    this->toggleKeyWasDown = toggleDown;

    // Same treatment for the mouse, sampled here because MonitorKeys runs once
    // per frame while MonitorMouse runs once per option
    bool leftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    this->leftPressed = leftDown && !this->leftWasDown;
    this->leftWasDown = leftDown;

    bool rightDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    this->rightPressed = rightDown && !this->rightWasDown;
    this->rightWasDown = rightDown;

    if (this->Ready())
    {
        // One key both shows and hides, from any submenu, matching how every
        // other overlay behaves.
        if (togglePressed)
        {
            this->open = !this->open;
            if (this->open)
                this->currentSub = MAIN_MENU;
            else
                this->SaveSettings();
            this->Wait(200);
        }
        if (GetAsyncKeyState(VK_BACK) & 0x10000)
        {
            this->CloseSub();
            this->Wait(200);
        }
    }
}

void RenderShader(float x, float y, float width, float height, float angle,
    const float *color, const char *material, int type)
{
    GameData::CG_DrawRotatedPicPhysical(GameData::scrPlace, x, y, width, height,
        angle, color, GameData::Material_RegisterHandle(material, type));
}

void DrawFillRect(float x, float y, float width, float height,
    const Colors::Color &color, float rotation, int type)
{
    RenderShader(x, y, width, height, rotation, color, "white", type);
}

void DrawEmptyRect(float x, float y, float width, float height, float size,
    const Colors::Color &color, int type)
{
    RenderShader(x, y, width, size, 0, color, "white", type); //up
    RenderShader(x, y + height, width + (size - 1), size, 0,  //down
        color, "white", type);
    RenderShader(x, y, size, height + (size - 1), 0, color,   //left
        "white", type);
    RenderShader(x + width, y, size, height + size, 0, color, //right
        "white", type);
}

float AlignText(const char *text, const Fonts::Font &font, float scale, float initX,
    ScreenAlignment align, bool ui, bool bg, GameData::Font_s **fOut,
    float *wOut, float *hOut)
{
    GameData::Font_s *fontPointer;
    float width, height;
    float xAligned;

    // Align text based on if callee requested text alignment for background (bool bg)
    // and wants text aligned using routines for all screen resolutions (bool ui)
    if (bg && ui)
    {
        fontPointer = GameData::UI_GetFontHandle(GameData::scrPlace, font.index);
        width = UI_TextWidth(text, INT_MAX, fontPointer, scale) + 12.0f;
        height = UI_TextHeight(fontPointer, scale) + 2.0f;
    }
    else if (!bg && ui)
    {
        fontPointer = GameData::UI_GetFontHandle(GameData::scrPlace, font.index);
        width = UI_TextWidth(text, INT_MAX, fontPointer, scale);
        height = UI_TextHeight(fontPointer, scale) + 2.0f;
    }
    else if (bg && !ui)
    {
        fontPointer = GameData::R_RegisterFont(font.dir, 0);
        width = R_TextWidth(text, INT_MAX, fontPointer) * scale + 14.0f;
        height = R_TextHeight(fontPointer) * scale + 2.0f;
    }
    else
    {
        fontPointer = GameData::R_RegisterFont(font.dir, 0);
        width = R_TextWidth(text, INT_MAX, fontPointer) * scale;
        height = R_TextHeight(fontPointer) * scale + 2.0f;
    }

    switch (align)
    {
        case ALIGN_LEFT:
            xAligned = initX;
            break;
        case ALIGN_CENTER:
            xAligned = initX - width / 2;
            break;
        case ALIGN_RIGHT:
            xAligned = initX - width;
            break;
        default:
            xAligned = initX;
            break;
    }

    if (wOut)
        *wOut = width;
    if (hOut)
        *hOut = height;
    if (fOut)
        *fOut = fontPointer;
    return xAligned;
}

float RenderGameText(const char *text, float x, float y, float scale,
    const Colors::Color &color, GameData::Font_s *font, float rotation)
{
    GameData::R_AddCmdDrawStretchPic(text, INT_MAX, font, x, y,
        scale, scale, rotation, color, 0);

    return GameData::R_TextHeight(font) * scale;
}

float RenderGameTextWithBackground(const char *text, float x, float y,
    float textW, float textH, const Colors::Color &borderColor, 
    const Colors::Color &textColor, GameData::Font_s *font, float scale)
{
    DrawFillRect(x, y - textH, textW - 2, textH + 2,
        Colors::transparentBlack, 0);
    DrawEmptyRect(x, y - textH, textW - 2, textH + 2, 2,
        borderColor);
    RenderGameText(text, x + 6, y, scale, textColor, font, 0);

    return textH + 2;
}

float RenderUIText(const std::string &text, float x, float y, float scale,
    const Colors::Color &color, GameData::Font_s *font)
{
    UI_DrawText(GameData::scrPlace, text.data(), INT_MAX, font, x, y,
        scale, 0.0f, color, 0);

    return UI_TextHeight(font, scale) + 2.0f;
}

float RenderUITextWithBackground(const char *text, float x, float y,
    float textW, float textH, const Colors::Color &borderColor, 
    const Colors::Color &textColor, GameData::Font_s *font, float scale)
{
    GameData::UI_FillRect(GameData::scrPlace, x, y - textH, textW - 2, textH + 2,
        0, 0, Colors::transparentBlack);
    GameData::UI_DrawRect(GameData::scrPlace, x, y - textH, textW - 2, textH + 2, 0, 0, 2, borderColor);
    RenderUIText(text, x + 6, y, scale, textColor, font);

    return textH + 2;
}

void WriteBytes(DWORD addr, const char *bytes, size_t len)
{
    DWORD curProtection;
    HANDLE curProcess = GetCurrentProcess();

    // Make virtual page address have read/write/exec privledges
    // and save old privledges to temporary
    VirtualProtect((LPVOID)addr, len, PAGE_EXECUTE_READWRITE, &curProtection);

    WriteProcessMemory(curProcess, (LPVOID)addr, bytes, len, 0);

    // Restore old privledges from temporary to virtual page address
    VirtualProtect((LPVOID)addr, len, curProtection, 0);

    // Run this to ensure instruction cache doesn't contain the instruction
    // from the address you just modified, therefore running the same instruction
    // you just removed
    FlushInstructionCache(curProcess, (void*)addr, len);
}

void ReadBytes(DWORD addr, char *buf, size_t len)
{
    // Make virtual page address have read/write/exec privledges
    // and save old privledges to temporary
    DWORD curProtection;
    VirtualProtect((LPVOID)addr, len, PAGE_EXECUTE_READWRITE, &curProtection);

    ReadProcessMemory(GetCurrentProcess(), (LPCVOID)addr, buf, len, 0);

    // Restore old privledges from temporary to virtual page address
    VirtualProtect((LPVOID)addr, len, curProtection, 0);
}

bool InsertDvar(const char *dvarName, GameData::dvar_s *dvar)
{
    GameData::dvar_s *pdvar = dvar ? dvar : GameData::Dvar_FindVar(dvarName);
    if (!pdvar)
        return false;

    dvars.insert(std::pair<std::string, GameData::dvar_s *>(dvarName, pdvar));
    return true;
}

bool InGame()
{
    return dvars.at("cl_ingame")->current.enabled
        && *GameData::cl_connectionState >= 9;
}

bool CopyTextToClipboard(const std::string &text)
{
    if (!OpenClipboard(*GameData::hwnd))
        return false;
    if (!EmptyClipboard())
        return false;

    size_t len = text.size() + 1;
    bool state = false;
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hg)
    {
        memcpy(static_cast<LPSTR>(GlobalLock(hg)), text.c_str(), len);
        GlobalUnlock(hg);
        state = SetClipboardData(CF_TEXT, hg);
    }

    CloseClipboard();
    GlobalFree(hg);
    return state;
}

bool CopyAddressToClipboard(void *address)
{
    std::stringstream str;
    str << std::hex << address;
    return CopyTextToClipboard(str.str());
}

std::string FormatError(DWORD lastError)
{
    LPSTR message;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message, 0, NULL);
    return message;
}
