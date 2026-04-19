#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <type_traits>
#include <limits>
#include <cstddef>

#define BTN_LEFT   1001
#define BTN_RIGHT  1002
#define BTN_VIEW   1003
#define BTN_APPLY  1004
#define EDIT_VALUE 2001

// ============================================================
// dummy global bitmap
// ============================================================
static const int BOX_BMP_W = 8;
static const int BOX_BMP_H = 8;

static unsigned int box_bitmap[BOX_BMP_W * BOX_BMP_H] = {
    0x00D8D8D8, 0x00C0C0C0, 0x00D8D8D8, 0x00C0C0C0, 0x00D8D8D8, 0x00C0C0C0, 0x00D8D8D8, 0x00C0C0C0,
    0x00C0C0C0, 0x00A8A8A8, 0x00C0C0C0, 0x00A8A8A8, 0x00C0C0C0, 0x00A8A8A8, 0x00C0C0C0, 0x00A8A8A8,
    0x00D8D8D8, 0x00C0C0C0, 0x00F4F4F4, 0x00E0E0E0, 0x00F4F4F4, 0x00E0E0E0, 0x00D8D8D8, 0x00C0C0C0,
    0x00C0C0C0, 0x00A8A8A8, 0x00E0E0E0, 0x00B8B8B8, 0x00E0E0E0, 0x00B8B8B8, 0x00C0C0C0, 0x00A8A8A8,
    0x00D8D8D8, 0x00C0C0C0, 0x00F4F4F4, 0x00E0E0E0, 0x00F4F4F4, 0x00E0E0E0, 0x00D8D8D8, 0x00C0C0C0,
    0x00C0C0C0, 0x00A8A8A8, 0x00E0E0E0, 0x00B8B8B8, 0x00E0E0E0, 0x00B8B8B8, 0x00C0C0C0, 0x00A8A8A8,
    0x00D8D8D8, 0x00C0C0C0, 0x00D8D8D8, 0x00C0C0C0, 0x00D8D8D8, 0x00C0C0C0, 0x00D8D8D8, 0x00C0C0C0,
    0x00888888, 0x00888888, 0x00888888, 0x00888888, 0x00888888, 0x00888888, 0x00888888, 0x00888888
};

static BITMAPINFO gBoxBmi;

// ============================================================
// dynamic load: user32 + gdi32
// ============================================================
typedef ATOM    (WINAPI *PFN_RegisterClassExA)(const WNDCLASSEXA*);
typedef HWND    (WINAPI *PFN_CreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef LRESULT (WINAPI *PFN_DefWindowProcA)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL    (WINAPI *PFN_ShowWindow)(HWND, int);
typedef BOOL    (WINAPI *PFN_UpdateWindow)(HWND);
typedef BOOL    (WINAPI *PFN_GetMessageA)(LPMSG, HWND, UINT, UINT);
typedef BOOL    (WINAPI *PFN_TranslateMessage)(const MSG*);
typedef LRESULT (WINAPI *PFN_DispatchMessageA)(const MSG*);
typedef VOID    (WINAPI *PFN_PostQuitMessage)(int);
typedef int     (WINAPI *PFN_MessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
typedef HCURSOR (WINAPI *PFN_LoadCursorA)(HINSTANCE, LPCSTR);
typedef BOOL    (WINAPI *PFN_InvalidateRect)(HWND, const RECT*, BOOL);
typedef HDC     (WINAPI *PFN_BeginPaint)(HWND, LPPAINTSTRUCT);
typedef BOOL    (WINAPI *PFN_EndPaint)(HWND, const PAINTSTRUCT*);
typedef BOOL    (WINAPI *PFN_GetClientRect)(HWND, LPRECT);
typedef int     (WINAPI *PFN_FillRect)(HDC, const RECT*, HBRUSH);
typedef BOOL    (WINAPI *PFN_SetWindowTextA)(HWND, LPCSTR);
typedef int     (WINAPI *PFN_GetWindowTextA)(HWND, LPSTR, int);

typedef int     (WINAPI *PFN_StretchDIBits)(HDC, int, int, int, int, int, int, int, int, const VOID*, const BITMAPINFO*, UINT, DWORD);
typedef HPEN    (WINAPI *PFN_CreatePen)(int, int, COLORREF);
typedef HGDIOBJ (WINAPI *PFN_SelectObject)(HDC, HGDIOBJ);
typedef BOOL    (WINAPI *PFN_DeleteObject)(HGDIOBJ);
typedef BOOL    (WINAPI *PFN_Rectangle)(HDC, int, int, int, int);
typedef BOOL    (WINAPI *PFN_MoveToEx)(HDC, int, int, LPPOINT);
typedef BOOL    (WINAPI *PFN_LineTo)(HDC, int, int);
typedef BOOL    (WINAPI *PFN_TextOutA)(HDC, int, int, LPCSTR, int);
typedef int     (WINAPI *PFN_SetBkMode)(HDC, int);
typedef COLORREF(WINAPI *PFN_SetTextColor)(HDC, COLORREF);

struct User32Api {
    HMODULE mod;
    PFN_RegisterClassExA RegisterClassExA_;
    PFN_CreateWindowExA  CreateWindowExA_;
    PFN_DefWindowProcA   DefWindowProcA_;
    PFN_ShowWindow       ShowWindow_;
    PFN_UpdateWindow     UpdateWindow_;
    PFN_GetMessageA      GetMessageA_;
    PFN_TranslateMessage TranslateMessage_;
    PFN_DispatchMessageA DispatchMessageA_;
    PFN_PostQuitMessage  PostQuitMessage_;
    PFN_MessageBoxA      MessageBoxA_;
    PFN_LoadCursorA      LoadCursorA_;
    PFN_InvalidateRect   InvalidateRect_;
    PFN_BeginPaint       BeginPaint_;
    PFN_EndPaint         EndPaint_;
    PFN_GetClientRect    GetClientRect_;
    PFN_FillRect         FillRect_;
    PFN_SetWindowTextA   SetWindowTextA_;
    PFN_GetWindowTextA   GetWindowTextA_;
} gUser32;

struct Gdi32Api {
    HMODULE mod;
    PFN_StretchDIBits StretchDIBits_;
    PFN_CreatePen     CreatePen_;
    PFN_SelectObject  SelectObject_;
    PFN_DeleteObject  DeleteObject_;
    PFN_Rectangle     Rectangle_;
    PFN_MoveToEx      MoveToEx_;
    PFN_LineTo        LineTo_;
    PFN_TextOutA      TextOutA_;
    PFN_SetBkMode     SetBkMode_;
    PFN_SetTextColor  SetTextColor_;
} gGdi32;

template <typename T>
static bool LoadProc(HMODULE mod, const char* name, T& out) {
    out = reinterpret_cast<T>(GetProcAddress(mod, name));
    return out != NULL;
}

static void InitBoxBitmapInfo() {
    std::memset(&gBoxBmi, 0, sizeof(gBoxBmi));
    gBoxBmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    gBoxBmi.bmiHeader.biWidth = BOX_BMP_W;
    gBoxBmi.bmiHeader.biHeight = -BOX_BMP_H;
    gBoxBmi.bmiHeader.biPlanes = 1;
    gBoxBmi.bmiHeader.biBitCount = 32;
    gBoxBmi.bmiHeader.biCompression = BI_RGB;
}

static bool InitDynamicApis() {
    std::memset(&gUser32, 0, sizeof(gUser32));
    std::memset(&gGdi32, 0, sizeof(gGdi32));

    gUser32.mod = LoadLibraryA("user32.dll");
    gGdi32.mod  = LoadLibraryA("gdi32.dll");
    if (!gUser32.mod || !gGdi32.mod) return false;

    bool ok = true;

    ok = ok && LoadProc(gUser32.mod, "RegisterClassExA", gUser32.RegisterClassExA_);
    ok = ok && LoadProc(gUser32.mod, "CreateWindowExA",  gUser32.CreateWindowExA_);
    ok = ok && LoadProc(gUser32.mod, "DefWindowProcA",   gUser32.DefWindowProcA_);
    ok = ok && LoadProc(gUser32.mod, "ShowWindow",       gUser32.ShowWindow_);
    ok = ok && LoadProc(gUser32.mod, "UpdateWindow",     gUser32.UpdateWindow_);
    ok = ok && LoadProc(gUser32.mod, "GetMessageA",      gUser32.GetMessageA_);
    ok = ok && LoadProc(gUser32.mod, "TranslateMessage", gUser32.TranslateMessage_);
    ok = ok && LoadProc(gUser32.mod, "DispatchMessageA", gUser32.DispatchMessageA_);
    ok = ok && LoadProc(gUser32.mod, "PostQuitMessage",  gUser32.PostQuitMessage_);
    ok = ok && LoadProc(gUser32.mod, "MessageBoxA",      gUser32.MessageBoxA_);
    ok = ok && LoadProc(gUser32.mod, "LoadCursorA",      gUser32.LoadCursorA_);
    ok = ok && LoadProc(gUser32.mod, "InvalidateRect",   gUser32.InvalidateRect_);
    ok = ok && LoadProc(gUser32.mod, "BeginPaint",       gUser32.BeginPaint_);
    ok = ok && LoadProc(gUser32.mod, "EndPaint",         gUser32.EndPaint_);
    ok = ok && LoadProc(gUser32.mod, "GetClientRect",    gUser32.GetClientRect_);
    ok = ok && LoadProc(gUser32.mod, "FillRect",         gUser32.FillRect_);
    ok = ok && LoadProc(gUser32.mod, "SetWindowTextA",   gUser32.SetWindowTextA_);
    ok = ok && LoadProc(gUser32.mod, "GetWindowTextA",   gUser32.GetWindowTextA_);

    ok = ok && LoadProc(gGdi32.mod, "StretchDIBits", gGdi32.StretchDIBits_);
    ok = ok && LoadProc(gGdi32.mod, "CreatePen",     gGdi32.CreatePen_);
    ok = ok && LoadProc(gGdi32.mod, "SelectObject",  gGdi32.SelectObject_);
    ok = ok && LoadProc(gGdi32.mod, "DeleteObject",  gGdi32.DeleteObject_);
    ok = ok && LoadProc(gGdi32.mod, "Rectangle",     gGdi32.Rectangle_);
    ok = ok && LoadProc(gGdi32.mod, "MoveToEx",      gGdi32.MoveToEx_);
    ok = ok && LoadProc(gGdi32.mod, "LineTo",        gGdi32.LineTo_);
    ok = ok && LoadProc(gGdi32.mod, "TextOutA",      gGdi32.TextOutA_);
    ok = ok && LoadProc(gGdi32.mod, "SetBkMode",     gGdi32.SetBkMode_);
    ok = ok && LoadProc(gGdi32.mod, "SetTextColor",  gGdi32.SetTextColor_);

    if (!ok) return false;

    InitBoxBitmapInfo();
    return true;
}

static void FreeDynamicApis() {
    if (gGdi32.mod) {
        FreeLibrary(gGdi32.mod);
        gGdi32.mod = NULL;
    }
    if (gUser32.mod) {
        FreeLibrary(gUser32.mod);
        gUser32.mod = NULL;
    }
}

// ============================================================
// watcher state
// ============================================================
struct WatchArrayState {
    const char* arrayName;
    void* data;
    size_t len;
    size_t selected;

    HWND hwndMain;
    HWND hwndEdit;

    bool (*getValueText)(void*, size_t, char*, int);
    bool (*setValueText)(void*, size_t, const char*, char*, int);
};

static WatchArrayState* gState = NULL;

// ============================================================
// helpers
// ============================================================
static const char* SkipSpaces(const char* s) {
    while (*s && std::isspace((unsigned char)*s)) ++s;
    return s;
}

static bool EqualsIgnoreCase(const char* a, const char* b) {
    while (*a && *b) {
        int ca = std::tolower((unsigned char)*a);
        int cb = std::tolower((unsigned char)*b);
        if (ca != cb) return false;
        ++a; ++b;
    }
    return (*a == '\0' && *b == '\0');
}

static void SetError(char* out, int cap, const char* msg) {
    if (!out || cap <= 0) return;
    std::snprintf(out, cap, "%s", msg);
}

// ============================================================
// type-tag dispatch (C++11, no if constexpr)
// ============================================================
struct BoolTag {};
struct FloatTag {};
struct SignedIntTag {};
struct UnsignedIntTag {};

template <typename T>
struct TypeTag {
    typedef typename std::conditional<
        std::is_same<T, bool>::value,
        BoolTag,
        typename std::conditional<
            std::is_floating_point<T>::value,
            FloatTag,
            typename std::conditional<
                std::is_signed<T>::value,
                SignedIntTag,
                UnsignedIntTag
            >::type
        >::type
    >::type type;
};

template <typename T>
static bool GetValueTextByTag(T* arr, size_t idx, char* out, int cap, BoolTag) {
    std::snprintf(out, cap, "%s", arr[idx] ? "true" : "false");
    return true;
}

template <typename T>
static bool GetValueTextByTag(T* arr, size_t idx, char* out, int cap, FloatTag) {
    std::snprintf(out, cap, "%.9g", (double)arr[idx]);
    return true;
}

template <typename T>
static bool GetValueTextByTag(T* arr, size_t idx, char* out, int cap, SignedIntTag) {
    std::snprintf(out, cap, "%lld", (long long)arr[idx]);
    return true;
}

template <typename T>
static bool GetValueTextByTag(T* arr, size_t idx, char* out, int cap, UnsignedIntTag) {
    std::snprintf(out, cap, "%llu", (unsigned long long)arr[idx]);
    return true;
}

template <typename T>
static bool SetValueTextByTag(T* arr, size_t idx, const char* text, char* err, int cap, BoolTag) {
    text = SkipSpaces(text);
    if (EqualsIgnoreCase(text, "true") || std::strcmp(text, "1") == 0) {
        arr[idx] = true;
        return true;
    }
    if (EqualsIgnoreCase(text, "false") || std::strcmp(text, "0") == 0) {
        arr[idx] = false;
        return true;
    }
    SetError(err, cap, "bool only accepts true/false/1/0");
    return false;
}

template <typename T>
static bool SetValueTextByTag(T* arr, size_t idx, const char* text, char* err, int cap, FloatTag) {
    char* end = NULL;
    double v = std::strtod(text, &end);
    if (end == text) {
        SetError(err, cap, "not a floating-point number");
        return false;
    }
    end = (char*)SkipSpaces(end);
    if (*end != '\0') {
        SetError(err, cap, "extra characters after number");
        return false;
    }
    arr[idx] = (T)v;
    return true;
}

template <typename T>
static bool SetValueTextByTag(T* arr, size_t idx, const char* text, char* err, int cap, SignedIntTag) {
    char* end = NULL;
    long long v = std::strtoll(text, &end, 0);
    if (end == text) {
        SetError(err, cap, "not a signed integer");
        return false;
    }
    end = (char*)SkipSpaces(end);
    if (*end != '\0') {
        SetError(err, cap, "extra characters after number");
        return false;
    }

    long long lo = (long long)std::numeric_limits<T>::lowest();
    long long hi = (long long)std::numeric_limits<T>::max();
    if (v < lo || v > hi) {
        SetError(err, cap, "value out of range");
        return false;
    }

    arr[idx] = (T)v;
    return true;
}

template <typename T>
static bool SetValueTextByTag(T* arr, size_t idx, const char* text, char* err, int cap, UnsignedIntTag) {
    char* end = NULL;
    unsigned long long v = std::strtoull(text, &end, 0);
    if (end == text) {
        SetError(err, cap, "not an unsigned integer");
        return false;
    }
    end = (char*)SkipSpaces(end);
    if (*end != '\0') {
        SetError(err, cap, "extra characters after number");
        return false;
    }

    unsigned long long hi = (unsigned long long)std::numeric_limits<T>::max();
    if (v > hi) {
        SetError(err, cap, "value out of range");
        return false;
    }

    arr[idx] = (T)v;
    return true;
}

template <typename T>
static bool GetValueTextImpl(void* data, size_t index, char* out, int cap) {
    T* arr = (T*)data;
    typename TypeTag<T>::type tag;
    return GetValueTextByTag(arr, index, out, cap, tag);
}

template <typename T>
static bool SetValueTextImpl(void* data, size_t index, const char* text, char* err, int cap) {
    T* arr = (T*)data;
    if (!text) {
        SetError(err, cap, "null input");
        return false;
    }
    text = SkipSpaces(text);
    if (*text == '\0') {
        SetError(err, cap, "empty input");
        return false;
    }

    typename TypeTag<T>::type tag;
    return SetValueTextByTag(arr, index, text, err, cap, tag);
}

// ============================================================
// GUI helpers
// ============================================================
static void RefreshEditAndRepaint() {
    if (!gState || !gState->hwndEdit || !gState->hwndMain) return;

    char buf[128];
    buf[0] = '\0';
    gState->getValueText(gState->data, gState->selected, buf, (int)sizeof(buf));
    gUser32.SetWindowTextA_(gState->hwndEdit, buf);
    gUser32.InvalidateRect_(gState->hwndMain, NULL, TRUE);
}

static void MoveLeft() {
    if (!gState) return;
    if (gState->selected > 0) --gState->selected;
    RefreshEditAndRepaint();
}

static void MoveRight() {
    if (!gState) return;
    if (gState->selected + 1 < gState->len) ++gState->selected;
    RefreshEditAndRepaint();
}

static void ShowCurrentValue(HWND hwnd) {
    if (!gState) return;

    char value[128];
    char msg[256];
    value[0] = '\0';
    msg[0] = '\0';

    gState->getValueText(gState->data, gState->selected, value, (int)sizeof(value));

    std::snprintf(
        msg, sizeof(msg),
        "array: %s\nindex: %llu\nvalue: %s",
        gState->arrayName,
        (unsigned long long)gState->selected,
        value
    );

    gUser32.MessageBoxA_(hwnd, msg, "View current box", MB_OK);
}

static void ApplyEdit(HWND hwnd) {
    if (!gState || !gState->hwndEdit) return;

    char text[128];
    char err[256];
    text[0] = '\0';
    err[0] = '\0';

    gUser32.GetWindowTextA_(gState->hwndEdit, text, (int)sizeof(text));

    if (!gState->setValueText(gState->data, gState->selected, text, err, (int)sizeof(err))) {
        gUser32.MessageBoxA_(hwnd, err, "Invalid value", MB_OK | MB_ICONERROR);
        RefreshEditAndRepaint();
        return;
    }

    RefreshEditAndRepaint();
}

static void DrawArrow(HDC hdc, int centerX, int topY) {
    HPEN pen = gGdi32.CreatePen_(PS_SOLID, 2, RGB(0, 90, 220));
    if (!pen) return;

    HGDIOBJ oldPen = gGdi32.SelectObject_(hdc, pen);

    int startY = topY - 20;
    int tipY   = topY - 4;

    gGdi32.MoveToEx_(hdc, centerX, startY, NULL);
    gGdi32.LineTo_(hdc, centerX, tipY);

    gGdi32.MoveToEx_(hdc, centerX - 6, tipY - 6, NULL);
    gGdi32.LineTo_(hdc, centerX, tipY);

    gGdi32.MoveToEx_(hdc, centerX + 6, tipY - 6, NULL);
    gGdi32.LineTo_(hdc, centerX, tipY);

    gGdi32.SelectObject_(hdc, oldPen);
    gGdi32.DeleteObject_(pen);
}

static void PaintMain(HWND hwnd) {
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc = gUser32.BeginPaint_(hwnd, &ps);

    gUser32.GetClientRect_(hwnd, &rc);
    gUser32.FillRect_(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

    if (!gState) {
        gUser32.EndPaint_(hwnd, &ps);
        return;
    }

    gGdi32.SetBkMode_(hdc, TRANSPARENT);
    gGdi32.SetTextColor_(hdc, RGB(20, 20, 20));

    char value[128];
    char header[256];
    value[0] = '\0';
    header[0] = '\0';

    gState->getValueText(gState->data, gState->selected, value, (int)sizeof(value));

    std::snprintf(
        header, sizeof(header),
        "%s | len=%llu | selected=%llu | value=%s",
        gState->arrayName,
        (unsigned long long)gState->len,
        (unsigned long long)gState->selected,
        value
    );
    gGdi32.TextOutA_(hdc, 10, 50, header, (int)std::strlen(header));

    int marginX = 10;
    int topY = 95;
    int cell = 48;
    int gap = 12;
    int labelH = 18;

    int contentW = (rc.right - rc.left) - marginX * 2;
    int perRow = contentW / (cell + gap);
    if (perRow < 1) perRow = 1;

    for (size_t i = 0; i < gState->len; ++i) {
        int row = (int)(i / (size_t)perRow);
        int col = (int)(i % (size_t)perRow);

        int x = marginX + col * (cell + gap);
        int y = topY + row * (cell + gap + labelH);

        gGdi32.StretchDIBits_(
            hdc,
            x, y, cell, cell,
            0, 0, BOX_BMP_W, BOX_BMP_H,
            box_bitmap,
            &gBoxBmi,
            DIB_RGB_COLORS,
            SRCCOPY
        );

        bool selected = (i == gState->selected);
        HPEN pen = gGdi32.CreatePen_(PS_SOLID, selected ? 3 : 1, selected ? RGB(220, 40, 40) : RGB(90, 90, 90));
        if (pen) {
            HGDIOBJ oldPen = gGdi32.SelectObject_(hdc, pen);
            gGdi32.Rectangle_(hdc, x, y, x + cell, y + cell);
            gGdi32.SelectObject_(hdc, oldPen);
            gGdi32.DeleteObject_(pen);
        }

        char idx[32];
        std::snprintf(idx, sizeof(idx), "[%llu]", (unsigned long long)i);
        gGdi32.TextOutA_(hdc, x + 6, y + cell + 2, idx, (int)std::strlen(idx));

        if (selected) {
            DrawArrow(hdc, x + cell / 2, y);
        }
    }

    gUser32.EndPaint_(hwnd, &ps);
}

// ============================================================
// window proc
// ============================================================
static LRESULT CALLBACK WatchWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
    {
        HINSTANCE hInst = GetModuleHandleA(NULL);
        gState->hwndMain = hwnd;

        gUser32.CreateWindowExA_(0, "BUTTON", "Left",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 10, 70, 28,
            hwnd, (HMENU)BTN_LEFT, hInst, NULL);

        gUser32.CreateWindowExA_(0, "BUTTON", "Right",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            90, 10, 70, 28,
            hwnd, (HMENU)BTN_RIGHT, hInst, NULL);

        gUser32.CreateWindowExA_(0, "BUTTON", "View",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            170, 10, 70, 28,
            hwnd, (HMENU)BTN_VIEW, hInst, NULL);

        gState->hwndEdit = gUser32.CreateWindowExA_(
            WS_EX_CLIENTEDGE,
            "EDIT", "",
            WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
            250, 10, 130, 28,
            hwnd, (HMENU)EDIT_VALUE, hInst, NULL
        );

        gUser32.CreateWindowExA_(0, "BUTTON", "Apply",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            390, 10, 70, 28,
            hwnd, (HMENU)BTN_APPLY, hInst, NULL);

        RefreshEditAndRepaint();
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case BTN_LEFT:  MoveLeft(); return 0;
        case BTN_RIGHT: MoveRight(); return 0;
        case BTN_VIEW:  ShowCurrentValue(hwnd); return 0;
        case BTN_APPLY: ApplyEdit(hwnd); return 0;
        }
        break;

    case WM_PAINT:
        PaintMain(hwnd);
        return 0;

    case WM_SIZE:
        gUser32.InvalidateRect_(hwnd, NULL, TRUE);
        return 0;

    case WM_DESTROY:
        gUser32.PostQuitMessage_(0);
        return 0;
    }

    return gUser32.DefWindowProcA_(hwnd, msg, wp, lp);
}

// ============================================================
// runner
// ============================================================
static int RunWatchArrayWindow(WatchArrayState& st) {
    if (!InitDynamicApis()) {
        std::fprintf(stderr, "InitDynamicApis() failed\n");
        return 1;
    }

    gState = &st;
    gState->selected = 0;
    gState->hwndMain = NULL;
    gState->hwndEdit = NULL;

    HINSTANCE hInst = GetModuleHandleA(NULL);

    WNDCLASSEXA wc;
    std::memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WatchWindowProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "WatchArrayWindowClass";
    wc.hCursor = gUser32.LoadCursorA_(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!gUser32.RegisterClassExA_(&wc)) {
        FreeDynamicApis();
        return 1;
    }

    char title[256];
    std::snprintf(title, sizeof(title), "WATCH_ARRAY - %s", st.arrayName);

    HWND hwnd = gUser32.CreateWindowExA_(
        0,
        "WatchArrayWindowClass",
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        100, 100, 760, 520,
        NULL, NULL, hInst, NULL
    );

    if (!hwnd) {
        FreeDynamicApis();
        return 1;
    }

    gUser32.ShowWindow_(hwnd, SW_SHOW);
    gUser32.UpdateWindow_(hwnd);

    MSG msg;
    while (gUser32.GetMessageA_(&msg, NULL, 0, 0) > 0) {
        gUser32.TranslateMessage_(&msg);
        gUser32.DispatchMessageA_(&msg);
    }

    FreeDynamicApis();
    gState = NULL;
    return (int)msg.wParam;
}

// ============================================================
// array-only macro
// if pointer -> compile error here
// ============================================================
template <typename T, size_t N>
char (&WatchArrayMustBeRealArray(T (&)[N]))[N];

template <typename T, size_t N>
int WatchArrayImpl(const char* name, T (&arr)[N]) {
    static_assert(!std::is_const<T>::value, "WATCH_ARRAY needs non-const array");
    static_assert(std::is_arithmetic<T>::value, "WATCH_ARRAY only supports basic arithmetic types");

    WatchArrayState st;
    st.arrayName = name;
    st.data = arr;
    st.len = N;
    st.selected = 0;
    st.hwndMain = NULL;
    st.hwndEdit = NULL;
    st.getValueText = &GetValueTextImpl<T>;
    st.setValueText = &SetValueTextImpl<T>;

    return RunWatchArrayWindow(st);
}

#define WATCH_ARRAY(arr) \
    ((void)sizeof(WatchArrayMustBeRealArray(arr)), WatchArrayImpl(#arr, (arr)))

// ============================================================
// demo
// ============================================================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    int demo[] = {10,20,30,40,50,60,70,80,90,100,110,120};

    // 這個會故意編譯失敗，因為是 pointer
    // int* p = demo;
    // WATCH_ARRAY(p);

    return WATCH_ARRAY(demo);
}
