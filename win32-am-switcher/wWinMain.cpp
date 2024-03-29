#include <sstream>
#include <string>

#include <Strsafe.h>
#include <Windows.h>

typedef std::basic_string<WCHAR> WSTRING;
typedef std::basic_stringstream<WCHAR> WSTREAM;

inline static const BOOL IS_JPN() {
    static const BOOL VALUE = (GetUserDefaultUILanguage() == 1041);
    return VALUE;
}

inline static const WSTRING ERROR_STRING(const WCHAR *ERROR_MESSAGE, const DWORD ERROR_CODE) {
    WSTREAM message_stream;
    const WSTRING MESSAGE = (ERROR_MESSAGE == NULL) ? L"An unknown error has occurred." : ERROR_MESSAGE;
    message_stream << MESSAGE << L"\nSystem error code is " << ERROR_CODE << L".";
    return message_stream.str();
}

static const struct {
    const WCHAR *title = L"Automatic Maintenance Switcher";
    int client_width = 270;
    int client_height = 88;
    COLORREF bg_color = RGB(232, 232, 232);
} WINDOW;

static const struct {
    HKEY root_key = HKEY_LOCAL_MACHINE;
    const WCHAR *sub_key = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\Maintenance";
    const WCHAR *value_name = L"MaintenanceDisabled";
    const DWORD value_false = 0;
    const DWORD value_true = 1;
} REGISTRY;

LRESULT CALLBACK MainWindowProcW(
    HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter);
void reflesh_automatic_maintenance_chechbox(HWND checkbox_handle);
void enable_automatic_maintenance();
void disable_automatic_maintenance();
void open_security_and_maintenance();

int APIENTRY wWinMain(
    _In_ HINSTANCE instance_handle, _In_opt_ HINSTANCE preview_instance_handle,
    _In_ WCHAR *command_line, _In_ int command_show_number
) {
    const WCHAR *WINDOW_CLASS_NAME = L"MainWindow";
    const WCHAR *ICON_NAME_ID = L"DefaultAppIcon";
    const UINT WINDOW_CLASS_STYLE = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

    WNDCLASS window_class = {};
    window_class.lpfnWndProc = MainWindowProcW;
    window_class.lpszClassName = WINDOW_CLASS_NAME;
    window_class.style = WINDOW_CLASS_STYLE;
    window_class.hInstance = instance_handle;
    window_class.hbrBackground = CreateSolidBrush(WINDOW.bg_color);
    window_class.hIcon = LoadIconW(instance_handle, ICON_NAME_ID);
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.lpszMenuName = NULL;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;

    if (RegisterClassW(&window_class) == 0) {
        const WCHAR *ERROR_MESSAGE = L"Failed to execute RegisterClassW().";
        MessageBoxW(NULL, ERROR_MESSAGE, L"Error", MB_OK | MB_ICONERROR);
        return 1;
    } else if (window_class.hIcon == NULL) {
        const WCHAR *WARNING_MESSAGE =
            L"The icon specified in the resources file is invalid. Windows default icon is used.";
        MessageBoxW(NULL, WARNING_MESSAGE, L"Warning", MB_OK | MB_ICONWARNING);
    }

    const DWORD WINDOW_STYLE = WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU;
    const DWORD EXTENDED_WINDOW_STYLE = WS_EX_APPWINDOW;
    const BOOL IS_EXISTS_MENU = FALSE;

    RECT window_rectangle = {};
    window_rectangle.left = 0;
    window_rectangle.right = WINDOW.client_width;
    window_rectangle.top = 0;
    window_rectangle.bottom = WINDOW.client_height;
    if (!AdjustWindowRectEx(&window_rectangle, WINDOW_STYLE, IS_EXISTS_MENU, EXTENDED_WINDOW_STYLE)) {
        const WCHAR *ERROR_MESSAGE = L"Failed to execute AdjustWindowRectEx().";
        MessageBoxW(NULL, ERROR_MESSAGE, L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    const int ADJUSTED_WIDTH = window_rectangle.right - window_rectangle.left;
    const int ADJUSTED_HEIGHT = window_rectangle.bottom - window_rectangle.top;
    const int DESKTOP_WIDTH = GetSystemMetrics(SM_CXSCREEN);
    const int DESKTOP_HEIGHT = GetSystemMetrics(SM_CYSCREEN);
    HWND window_handle = CreateWindowExW(
        EXTENDED_WINDOW_STYLE, WINDOW_CLASS_NAME, WINDOW.title, WINDOW_STYLE,
        (DESKTOP_WIDTH - ADJUSTED_WIDTH) / 2, (DESKTOP_HEIGHT - ADJUSTED_HEIGHT) / 2,
        ADJUSTED_WIDTH, ADJUSTED_HEIGHT, NULL, NULL, instance_handle, NULL);
    if (window_handle == NULL) {
        const WCHAR *ERROR_MESSAGE = L"Failed to execute CreateWindowExW().";
        MessageBoxW(NULL, ERROR_MESSAGE, L"Error", MB_OK | MB_ICONERROR);
        return 1;
    } else {
        ShowWindow(window_handle, SW_SHOW);
        UpdateWindow(window_handle);
    }

    MSG message = {};
    while (GetMessageW(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK MainWindowProcW(
    HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter
) {
    static const HINSTANCE INSTANCE_HANDLE = static_cast<HINSTANCE>(GetModuleHandleW(NULL));

    static struct {
        RECT window = {};
        RECT client = {};
    } rectangle;

    static struct {
        const HFONT FONT = CreateFontW(
            (!IS_JPN()) ? 17 : 21, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
            (!IS_JPN()) ? ANSI_CHARSET : SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE, (!IS_JPN()) ? L"Arial" : L"メイリオ");
        HWND checkbox = NULL;
        HWND button = NULL;
    } handle;

    static const struct {
        HMENU checkbox = reinterpret_cast<HMENU>(10);
        HMENU button = reinterpret_cast<HMENU>(20);
    } MENU_ID;

    try {
        if (message == WM_CREATE) {
            if (handle.FONT == NULL) {
                throw ERROR_STRING(L"Failed to create default font.", GetLastError());
            }
            struct {
                int x = 0, y = 0, w = 0, h = 0;
            } form;
            GetWindowRect(window_handle, &rectangle.window);
            GetClientRect(window_handle, &rectangle.client);

            const int FORM_MARGIN = 16;
            const int CHILD_WINDOW_HEIGHT = 24;
            const int CHILD_WINDOW_WIDTH = (rectangle.client.right - rectangle.client.left) - FORM_MARGIN * 2;
            const int CHECKBOX_VERTICAL_OFFSET = 2;
            const int CHECKBOX_HORIZONTAL_OFFSET = 4;

            const DWORD CHECKBOX_STYLE = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_LEFT | BS_VCENTER | BS_MULTILINE;
            const WCHAR *CHECKBOX_TEXT = (!IS_JPN()) ? L"Disable Automatic Maintenance" : L"自動メンテナンスを無効にする";
            form.x = FORM_MARGIN;
            form.y = FORM_MARGIN;
            form.w = CHILD_WINDOW_WIDTH;
            form.h = CHILD_WINDOW_HEIGHT;
            handle.checkbox = CreateWindowExW(
                NULL, L"BUTTON", CHECKBOX_TEXT, CHECKBOX_STYLE,
                form.x + CHECKBOX_HORIZONTAL_OFFSET, form.y - CHECKBOX_VERTICAL_OFFSET,
                form.w - CHECKBOX_HORIZONTAL_OFFSET * 2, form.h, window_handle, MENU_ID.checkbox, INSTANCE_HANDLE, NULL);
            SendMessageW(handle.checkbox, WM_SETFONT, reinterpret_cast<WPARAM>(handle.FONT), true);

            const DWORD BUTTON_STYLE = WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER | BS_MULTILINE;
            const WCHAR *BUTTON_TEXT = (!IS_JPN()) ? L"Security and Maintenance" : L"セキュリティとメンテナンス";
            form.y += CHILD_WINDOW_HEIGHT + FORM_MARGIN / 2;
            handle.button = CreateWindowExW(
                NULL, L"BUTTON", BUTTON_TEXT, BUTTON_STYLE,
                form.x, form.y, form.w, form.h, window_handle, MENU_ID.button, INSTANCE_HANDLE, NULL);
            SendMessageW(handle.button, WM_SETFONT, reinterpret_cast<WPARAM>(handle.FONT), true);

            reflesh_automatic_maintenance_chechbox(handle.checkbox);
            return static_cast<LRESULT>(0);
        }
        if (message == WM_CTLCOLORBTN || message == WM_CTLCOLORSTATIC) {
            SetBkColor(reinterpret_cast<HDC>(word_parameter), WINDOW.bg_color);
            return reinterpret_cast<LRESULT>(CreateSolidBrush(WINDOW.bg_color));
        }
        if (message == WM_COMMAND) {
            SetFocus(NULL);
            const HMENU COMMAND_ID = reinterpret_cast<HMENU>(LOWORD(word_parameter));
            if (COMMAND_ID == MENU_ID.checkbox) {
                if (SendMessageW(handle.checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    const WCHAR *MESSAGE_TEXT = (!IS_JPN()) ?
                        L"Do you want to \"disable\" Automatic Maintenance?\n"
                        L"It is recommended to run Automatic Maintenance\n"
                        L"manually in advance, and then disable it when complete." :
                        L"自動メンテナンスを「無効」にしますか？\n"
                        L"あらかじめ自動メンテナンスを手動で実行させて、\n"
                        L"完了した後に無効にすることをお勧めします。";
                    const WCHAR *MESSAGE_TITLE = (!IS_JPN()) ? L"Confirm" : L"確認";
                    const int SELECTION = MessageBoxW(
                        window_handle, MESSAGE_TEXT, MESSAGE_TITLE, MB_YESNO | MB_ICONQUESTION);
                    if (SELECTION == IDYES) {
                        disable_automatic_maintenance();
                        const WCHAR *MESSAGE_TEXT = (!IS_JPN()) ?
                            L"Automatic Maintenance has been \"disabled\"." :
                            L"自動メンテナンスを「無効」に設定しました。";
                        const WCHAR *MESSAGE_TITLE = (!IS_JPN()) ? L"Success" : L"完了";
                        MessageBoxW(NULL, MESSAGE_TEXT, MESSAGE_TITLE, MB_OK | MB_ICONINFORMATION);
                    }
                } else {
                    const WCHAR *MESSAGE_TEXT = (!IS_JPN()) ?
                        L"Do you want to \"enable\" Automatic Maintenance?\n"
                        L"If you want to perform processing with high CPU load,\n"
                        L"try disabling it again." :
                        L"自動メンテナンスを「有効」にしますか？\n"
                        L"CPUの負荷が大きい処理を行う場合は、\n"
                        L"改めて無効にしてみてください。";
                    const WCHAR *MESSAGE_TITLE = (!IS_JPN()) ? L"Confirm" : L"確認";
                    const int SELECTION = MessageBoxW(
                        window_handle, MESSAGE_TEXT, MESSAGE_TITLE, MB_YESNO | MB_ICONQUESTION);
                    if (SELECTION == IDYES) {
                        enable_automatic_maintenance();
                        const WCHAR *MESSAGE_TEXT = (!IS_JPN()) ?
                            L"Automatic Maintenance has been \"enabled\"." :
                            L"自動メンテナンスを「有効」に設定しました。";
                        const WCHAR *MESSAGE_TITLE = (!IS_JPN()) ? L"Success" : L"完了";
                        MessageBoxW(NULL, MESSAGE_TEXT, MESSAGE_TITLE, MB_OK | MB_ICONINFORMATION);
                    }
                }
                reflesh_automatic_maintenance_chechbox(handle.checkbox);
            }
            if (COMMAND_ID == MENU_ID.button) {
                const WCHAR *MESSAGE_TEXT = (!IS_JPN()) ?
                    L"Do you want to open Security and Maintenance (Control Panel) ?\n"
                    L"If you want to manually run Automatic Maintenance,\n"
                    L"expand the \"Maintenance\" item and click \"Start Maintenance\"." :
                    L"セキュリティとメンテナンス（コントロールパネル）を開きますか？\n"
                    L"自動メンテナンスを手動で実行したい場合は、「メンテナンス(M)」\n"
                    L"の項目を展開して、「メンテナンスの開始」をクリックします。";
                const WCHAR *MESSAGE_TITLE = (!IS_JPN()) ? L"Confirm" : L"確認";
                const int SELECTION = MessageBoxW(
                    window_handle, MESSAGE_TEXT, MESSAGE_TITLE, MB_YESNO | MB_ICONQUESTION);
                if (SELECTION == IDYES) {
                    open_security_and_maintenance();
                }
            }
            return static_cast<LRESULT>(0);
        }
        if (message == WM_SIZE) {
            const int CLIENT_WIDTH = LOWORD(long_parameter);
            const int CLIENT_HEIGHT = HIWORD(long_parameter);
            const int INITIAL_WIDTH = rectangle.client.right - rectangle.client.left;
            const int INITIAL_HEIGHT = rectangle.client.bottom - rectangle.client.top;
            if (CLIENT_WIDTH != INITIAL_WIDTH || CLIENT_HEIGHT != INITIAL_HEIGHT) {
                const int WINDOW_WIDTH = rectangle.window.right - rectangle.window.left;
                const int WINDOW_HEIGHT = rectangle.window.bottom - rectangle.window.top;
                SetWindowPos(window_handle, HWND_TOP, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_NOMOVE);
            }
            return static_cast<LRESULT>(0);
        }
        if (message == WM_CLOSE) {
            DestroyWindow(window_handle);
            return static_cast<LRESULT>(0);
        }
        if (message == WM_DESTROY) {
            DeleteObject(static_cast<HGDIOBJ>(handle.FONT));
            PostQuitMessage(0);
            return static_cast<LRESULT>(0);
        }
    } catch (WSTRING &error) {
        MessageBoxW(window_handle, error.c_str(), L"Error", MB_OK | MB_ICONERROR);
        DestroyWindow(window_handle);
        return static_cast<LRESULT>(0);
    } catch (...) {
        MessageBoxW(window_handle, ERROR_STRING(NULL, GetLastError()).c_str(), L"Error", MB_OK | MB_ICONERROR);
        DestroyWindow(window_handle);
        return static_cast<LRESULT>(0);
    }
    return DefWindowProcW(window_handle, message, word_parameter, long_parameter);
}

void reflesh_automatic_maintenance_chechbox(HWND checkbox_handle) {
    HKEY registry_handle = {};
    const LSTATUS OPEN_RESULT = RegOpenKeyExW(
        REGISTRY.root_key, REGISTRY.sub_key, 0, KEY_READ | KEY_WOW64_64KEY, &registry_handle);
    if (OPEN_RESULT == ERROR_SUCCESS) {
        DWORD value_type = REG_NONE;
        DWORD value_data = 0;
        DWORD value_data_size = sizeof(DWORD);
        const LSTATUS READ_RESULT = RegQueryValueExW(
            registry_handle, REGISTRY.value_name, NULL, &value_type,
            reinterpret_cast<BYTE *>(&value_data), &value_data_size);
        if (READ_RESULT == ERROR_SUCCESS) {
            if (value_data == REGISTRY.value_false) {
                SendMessageW(checkbox_handle, BM_SETCHECK, BST_UNCHECKED, 0);
            } else {
                SendMessageW(checkbox_handle, BM_SETCHECK, BST_CHECKED, 0);
            }
        } else if (READ_RESULT == ERROR_FILE_NOT_FOUND) {
            SendMessageW(checkbox_handle, BM_SETCHECK, BST_UNCHECKED, 0);
        } else {
            RegCloseKey(registry_handle);
            throw ERROR_STRING(L"Failed to read registry.", READ_RESULT);
        }
    } else if (OPEN_RESULT == ERROR_ACCESS_DENIED) {
        RegCloseKey(registry_handle);
        throw ERROR_STRING(L"Requires administrator privileges.", OPEN_RESULT);
    } else {
        RegCloseKey(registry_handle);
        throw ERROR_STRING(L"Failed to open registry key.", OPEN_RESULT);
    }
    RegCloseKey(registry_handle);
    return;
}

void enable_automatic_maintenance() {
    HKEY registry_handle = {};
    const LSTATUS OPEN_RESULT = RegOpenKeyExW(
        REGISTRY.root_key, REGISTRY.sub_key, 0, KEY_WRITE | KEY_WOW64_64KEY, &registry_handle);
    if (OPEN_RESULT == ERROR_SUCCESS) {
        const LSTATUS DELETE_RESULT = RegDeleteValueW(registry_handle, REGISTRY.value_name);
        if (DELETE_RESULT != ERROR_SUCCESS) {
            RegCloseKey(registry_handle);
            throw ERROR_STRING(L"Failed to delete registry key.", DELETE_RESULT);
        }
    } else if (OPEN_RESULT == ERROR_ACCESS_DENIED) {
        RegCloseKey(registry_handle);
        throw ERROR_STRING(L"Requires administrator privileges.", OPEN_RESULT);
    } else {
        RegCloseKey(registry_handle);
        throw ERROR_STRING(L"Failed to open registry key.", OPEN_RESULT);
    }
    RegCloseKey(registry_handle);
    return;
}

void disable_automatic_maintenance() {
    HKEY registry_handle = {};
    const LSTATUS OPEN_RESULT = RegOpenKeyExW(
        REGISTRY.root_key, REGISTRY.sub_key, 0, KEY_WRITE | KEY_WOW64_64KEY, &registry_handle);
    if (OPEN_RESULT == ERROR_SUCCESS) {
        DWORD value_data = REGISTRY.value_true;
        const LSTATUS WRITE_RESULT = RegSetValueExW(
            registry_handle, REGISTRY.value_name, 0, REG_DWORD,
            reinterpret_cast<BYTE *>(&value_data), sizeof(value_data));
        if (WRITE_RESULT != ERROR_SUCCESS) {
            RegCloseKey(registry_handle);
            throw ERROR_STRING(L"Failed to write registry key.", WRITE_RESULT);
        }
    } else if (OPEN_RESULT == ERROR_ACCESS_DENIED) {
        RegCloseKey(registry_handle);
        throw ERROR_STRING(L"Requires administrator privileges.", OPEN_RESULT);
    } else {
        RegCloseKey(registry_handle);
        throw ERROR_STRING(L"Failed to open registry key.", OPEN_RESULT);
    }
    RegCloseKey(registry_handle);
    return;
}

void open_security_and_maintenance() {
    WSTRING command_string = L"C:\\Windows\\System32\\control.exe -name Microsoft.ActionCenter";
    WCHAR command[MAX_PATH] = L"";
    const HRESULT RESULT = StringCbCopyExW(
        command, sizeof(WCHAR) * MAX_PATH, command_string.c_str(), NULL, NULL, STRSAFE_NULL_ON_FAILURE);
    if (RESULT != S_OK) {
        throw ERROR_STRING(L"Failed to copy character array.", static_cast<DWORD>(RESULT));
    }
    STARTUPINFO startup_info = {};
    PROCESS_INFORMATION process_info = {};
    const BOOL IS_PROCESS_SUCCEED = CreateProcessW(
        NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    if (!IS_PROCESS_SUCCEED) {
        throw ERROR_STRING(L"Failed to open Control Panel.", GetLastError());
    }
    return;
}
