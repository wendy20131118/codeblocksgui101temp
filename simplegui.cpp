#include <windows.h>
#define main() WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow)
// 1. 視窗過程 (Window Procedure)：處理訊息的地方（按鈕點擊、關閉視窗等）
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0); // 關閉程式
            break;
        case WM_CREATE:
            // 在視窗裡放一個按鈕
            CreateWindow("BUTTON", "click me",
                         WS_VISIBLE | WS_CHILD,
                         50, 50, 100, 30,
                         hwnd, (HMENU)1, NULL, NULL);
            break;
        case WM_COMMAND:
            if (LOWORD(wp) == 1) { // 如果按鈕 ID 是 1
                MessageBox(hwnd, "you clicked", "note", MB_OK);
            }
            break;
        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

// 2. 主程式入口 (WinMain)
int main() {
    WNDCLASSW wc = {0};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = L"myWindowClass";
    wc.lpfnWndProc = WindowProcedure;

    // 註冊視窗類別
    if (!RegisterClassW(&wc)) return -1;

    // 建立視窗
    CreateWindowW(L"myWindowClass", L"some shit",
                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                  100, 100, 500, 500,
                  NULL, NULL, hInst, NULL);

    // 3. 訊息迴圈 (Message Loop)：程式會在這裡不斷循環，等待使用者操作
    MSG msg = {0};
    while (GetMessage(&msg, NULL, NULL, NULL)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
