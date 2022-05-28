#include <windows.h>
#define FILE_MENU_OPEN 1
#define FILE_MENU_EXIT 2
#define HELP_MENU_ABOUT 3
#define FILE_MENU_SAVE_AS 4
#define FORMAT_MENU_FONT 5

void AddMenus(HWND);
HMENU hMenu;

typedef struct {
    wchar_t filename[MAX_PATH];
} DOCUMENT_STATE;

typedef struct {
    DOCUMENT_STATE document;
    HWND hWnd;
    HWND hWndEdit;
    HINSTANCE hInstance;
    HICON hIcon;
    HFONT hFont;
} APP_STATE;

APP_STATE app;

const wchar_t* NOTEPAD_WNDCLASS = L"Notepad";

BOOL Notepad_OpenFile(const wchar_t* filename) {
    BOOL success = FALSE;
    BYTE* fileBuffer = (BYTE*)GlobalAlloc(GMEM_FIXED, 1024 * 1024);
    BYTE* filePosition = fileBuffer;
    // Open the file handle
    HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        // Read the contents of the file
        enum { BUFSIZE = 4096 };
        BYTE readBuffer[BUFSIZE];
        DWORD bytesRead = 0;

        while (ReadFile(file, readBuffer, BUFSIZE, &bytesRead, NULL) && (bytesRead != 0)) {
            memcpy(filePosition, readBuffer, bytesRead);
            filePosition += bytesRead;
        }

        *filePosition = 0;


        if (filePosition != fileBuffer) {
            SetWindowTextA(app.hWndEdit, (char*)fileBuffer);
            success = TRUE;
        }
        // Insert the contents into the edit control
        // If everything is okay, return TRUE
        CloseHandle(file);
    }
    GlobalFree(fileBuffer);
    return success;
}

BOOL Notepad_SaveAsFile(const wchar_t* filename) {
    BOOL success = FALSE;
    enum { BUFSIZE = 1024 * 1024 };
    char* fileBuffer = (char*)GlobalAlloc(GMEM_FIXED, BUFSIZE);
    int length = GetWindowTextA(app.hWndEdit, fileBuffer, BUFSIZE);
    if (length != 0) {
        HANDLE file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE) {
            DWORD bytesWritten = 0;
            if (WriteFile(file, fileBuffer, length, &bytesWritten, NULL)) {
                if (bytesWritten == length) {
                    success = TRUE;
                }
            }
            CloseHandle(file);
        }
    }
    GlobalFree(fileBuffer);
    return success;
}

LRESULT CALLBACK Notepad_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        switch (wParam) {
        case FILE_MENU_EXIT: {
            DestroyWindow(hWnd);
            break;
        }
        case FILE_MENU_OPEN: {
            wchar_t filenameBuffer[MAX_PATH];
            ZeroMemory(&filenameBuffer, MAX_PATH * sizeof(wchar_t));
            
            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hInstance = app.hInstance;
            ofn.hwndOwner = hWnd;
            ofn.lpstrTitle = L"Open File";
            ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = filenameBuffer;
            ofn.nMaxFile = MAX_PATH;

            if (GetOpenFileName(&ofn)) {
                Notepad_OpenFile(filenameBuffer);
            }
            break;
        }
        case HELP_MENU_ABOUT: {
            const wchar_t* aboutText = L"This is a Notepad Clone built with C++";
            MessageBox(NULL, aboutText, L"About", MB_OK);
            break;
        }
        case FILE_MENU_SAVE_AS: {
            wchar_t filenameBuffer[MAX_PATH];
            ZeroMemory(&filenameBuffer, MAX_PATH * sizeof(wchar_t));

            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hInstance = app.hInstance;
            ofn.hwndOwner = hWnd;
            ofn.lpstrTitle = L"Save As";
            ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = filenameBuffer;
            ofn.nMaxFile = MAX_PATH;

            if (GetSaveFileName(&ofn)) {
                Notepad_SaveAsFile(filenameBuffer);
            }
            break;
        } case FORMAT_MENU_FONT: {
            LOGFONT logfont = { 0 };

            CHOOSEFONT cf = { 0 };
            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = hWnd;
            cf.lpLogFont = &logfont;
            cf.hInstance = app.hInstance;
            cf.nFontType = SCREEN_FONTTYPE;
            cf.Flags = CF_SCREENFONTS;
            
            if (ChooseFont(&cf)) {
                app.hFont = CreateFontIndirect(&logfont);
                SendMessage(app.hWndEdit, WM_SETFONT, (WPARAM)app.hFont, TRUE);
            }
            break;
        }
        } 

        break;
    }
    case WM_CREATE: {
        app.hWndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL,
            0, 0, 0, 0, hWnd, NULL, app.hInstance, NULL);

        AddMenus(hWnd);
        ShowWindow(app.hWndEdit, SW_SHOW);
        ShowWindow(hWnd, SW_SHOW);
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
    case WM_SIZE: {
        RECT rect;
        int width;
        int height;
        GetClientRect(hWnd, &rect);

        width = rect.right - rect.left;
        height = rect.bottom - rect.top;

        MoveWindow(app.hWndEdit, 0, 0, width, height, FALSE);

        break;
    }
    case WM_CLOSE: {
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY: {
        if (app.hFont != NULL) {
            DeleteObject(app.hFont);
        }
        PostQuitMessage(0);
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL Notepad_RegisterClass(HINSTANCE instance) {
    WNDCLASSEX wc = { 0 };

    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Notepad_WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = NOTEPAD_WNDCLASS;

    return (RegisterClassEx(&wc) != 0);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, int show) {
    if (!Notepad_RegisterClass(instance)) {
        MessageBox(NULL, L"Couldn't register window class!", L"Error", MB_OK | MB_ICONERROR);
    }

    ZeroMemory(&app, sizeof(APP_STATE));
    app.hInstance = instance;
    app.hWnd = CreateWindowEx(0, NOTEPAD_WNDCLASS, L"Notepad", WS_OVERLAPPEDWINDOW,
        0, 0, 640, 480, NULL, NULL, instance, NULL);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void AddMenus(HWND hWnd) {
    hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hHelpMenu = CreateMenu();
    HMENU hFormatMenu = CreateMenu();

    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_OPEN, L"Open");
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_SAVE_AS, L"Save as");
    AppendMenu(hFileMenu, MF_SEPARATOR, (UINT_PTR)nullptr, NULL);
    AppendMenu(hFileMenu, MF_STRING, FILE_MENU_EXIT, L"Exit");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFormatMenu, L"Format");
    AppendMenu(hFormatMenu, MF_STRING, FORMAT_MENU_FONT, L"Font");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Help");
    AppendMenu(hHelpMenu, MF_STRING, HELP_MENU_ABOUT, L"About");

    SetMenu(hWnd, hMenu);

}
