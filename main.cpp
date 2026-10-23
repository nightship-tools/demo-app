// nightship-demo: a tiny Windows app that exists to be delivered by Nightship.
//
// It shows one line of text, very large, on a dark window - by default the
// contents of message.txt next to the exe ("Hello Demo"). Change that file,
// publish a new build, press Run in the Nightship client: the new text appears.
//
//   nightship-demo.exe                  -> text from message.txt
//   nightship-demo.exe --text "Hello"   -> this text instead (one launch target
//                                          per greeting = several Run buttons)
//
// A small line underneath names the build (build.txt, written by CI) so it is
// always clear which build is running. Plain Win32, static CRT, no other DLLs.

#ifndef UNICODE
#  define UNICODE
#endif
#ifndef _UNICODE
#  define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>

#include <fstream>
#include <sstream>
#include <string>

namespace {

std::wstring exeDir()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring p(path);
    const size_t slash = p.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring() : p.substr(0, slash + 1);
}

// First line of a UTF-8 text file beside the exe, or empty.
std::wstring readLine(const wchar_t* name)
{
    std::ifstream f(exeDir() + name, std::ios::binary);
    if (!f)
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB
        && (unsigned char)s[2] == 0xBF)
        s.erase(0, 3);                                   // UTF-8 BOM
    const size_t nl = s.find_first_of("\r\n");
    if (nl != std::string::npos)
        s.resize(nl);
    if (s.empty())
        return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::wstring g_text;
std::wstring g_footer;

const COLORREF kNightTop = RGB(0x10, 0x1a, 0x30);
const COLORREF kNightBottom = RGB(0x06, 0x0a, 0x14);
const COLORREF kInk = RGB(0xdc, 0xe7, 0xf5);
const COLORREF kInkDim = RGB(0x8f, 0xa3, 0xbf);
const COLORREF kAccent = RGB(0x35, 0xb6, 0xd4);

HFONT makeFont(int height, int weight)
{
    return CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

void paint(HWND hwnd, HDC target)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    const int w = rc.right, h = rc.bottom;
    if (w <= 0 || h <= 0)
        return;

    // Draw into a back buffer so resizing never flickers.
    HDC dc = CreateCompatibleDC(target);
    HBITMAP bmp = CreateCompatibleBitmap(target, w, h);
    HGDIOBJ oldBmp = SelectObject(dc, bmp);

    // Night gradient, top to bottom.
    TRIVERTEX v[2] = {
        {0, 0, (COLOR16)(GetRValue(kNightTop) << 8), (COLOR16)(GetGValue(kNightTop) << 8),
         (COLOR16)(GetBValue(kNightTop) << 8), 0},
        {w, h, (COLOR16)(GetRValue(kNightBottom) << 8), (COLOR16)(GetGValue(kNightBottom) << 8),
         (COLOR16)(GetBValue(kNightBottom) << 8), 0}};
    GRADIENT_RECT gr = {0, 1};
    GradientFill(dc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);
    SetBkMode(dc, TRANSPARENT);

    // Main text: as large as fits in 88% of the width, at most ~28% of the height.
    const int maxW = w * 88 / 100;
    int size = h * 28 / 100;
    HFONT bigFont = nullptr;
    SIZE ext = {};
    for (; size > 12; size = size * 92 / 100) {
        if (bigFont)
            DeleteObject(bigFont);
        bigFont = makeFont(size, FW_BOLD);
        SelectObject(dc, bigFont);
        GetTextExtentPoint32W(dc, g_text.c_str(), (int)g_text.size(), &ext);
        if (ext.cx <= maxW)
            break;
    }
    SetTextColor(dc, kInk);
    const int textY = (h - ext.cy) / 2 - h / 20;
    TextOutW(dc, (w - ext.cx) / 2, textY, g_text.c_str(), (int)g_text.size());

    // Accent bar under the text.
    const int barW = w / 10, barH = (h / 120) > 2 ? h / 120 : 2;
    RECT bar = {(w - barW) / 2, textY + ext.cy + h / 30, (w + barW) / 2, 0};
    bar.bottom = bar.top + barH;
    HBRUSH accent = CreateSolidBrush(kAccent);
    FillRect(dc, &bar, accent);
    DeleteObject(accent);

    // Footer: which build is running.
    HFONT footerFont = makeFont((h / 28) > 12 ? h / 28 : 12, FW_NORMAL);
    SelectObject(dc, footerFont);
    SetTextColor(dc, kInkDim);
    SIZE fext = {};
    GetTextExtentPoint32W(dc, g_footer.c_str(), (int)g_footer.size(), &fext);
    TextOutW(dc, (w - fext.cx) / 2, bar.bottom + h / 25, g_footer.c_str(),
             (int)g_footer.size());

    BitBlt(target, 0, 0, w, h, dc, 0, 0, SRCCOPY);
    SelectObject(dc, oldBmp);
    DeleteObject(bmp);
    DeleteObject(bigFont);
    DeleteObject(footerFont);
    DeleteDC(dc);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;                       // painted fully in WM_PAINT
    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        paint(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // --text "..." overrides message.txt.
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; argv && i + 1 < argc; ++i)
        if (std::wstring(argv[i]) == L"--text")
            g_text = argv[i + 1];
    if (argv)
        LocalFree(argv);
    if (g_text.empty())
        g_text = readLine(L"message.txt");
    if (g_text.empty())
        g_text = L"Hello Demo";

    std::wstring build = readLine(L"build.txt");
    g_footer = (build.empty() ? std::wstring(L"local build") : L"build " + build)
             + L"  \u00B7  delivered by Nightship";

    const wchar_t* kClass = L"NightshipDemo";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = kClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    const int ww = 1100, wh = 620;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;
    HWND hwnd = CreateWindowExW(0, kClass, L"Nightship Demo", WS_OVERLAPPEDWINDOW,
                                x, y, ww, wh, nullptr, nullptr, hInst, nullptr);
    if (!hwnd)
        return 1;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
