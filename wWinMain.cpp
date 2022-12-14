#include <Windows.h>
#include <sstream>
#include <string>

typedef std::basic_stringstream<WCHAR> WSTREAM;
typedef std::basic_string<WCHAR> WSTRING;

inline const WCHAR *LANG_TEXT(const WCHAR *en_us, const WCHAR *ja_jp) {
    static LANGID language_id = GetUserDefaultUILanguage();
    return (language_id != 1041) ? en_us : ja_jp;
}

static const WCHAR *WINDOW_TITLE = L"Automatic Maintenance Switcher";
static const WCHAR *SUB_KEY = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\Maintenance";
static const WCHAR *VALUE_NAME = L"MaintenanceDisabled";

bool is_automatic_maintenance_disabled();
void enable_automatic_maintenance();
void disable_automatic_maintenance();

int APIENTRY wWinMain(
    _In_ HINSTANCE instance_handle, _In_opt_ HINSTANCE preview_instance_handle,
    _In_ WCHAR *command_line, _In_ int command_show_number
) {
    try {
        if (is_automatic_maintenance_disabled()) {
            const WCHAR *message_before_enable = LANG_TEXT(
                L"Automatic maintenance is \"disabled\".\nDo you want to enable it?",
                L"自動メンテナンスは「無効」に設定されています。\n有効にしますか？");
            int selection = MessageBoxW(NULL, message_before_enable, WINDOW_TITLE, MB_YESNO | MB_ICONQUESTION);
            if (selection == IDYES) {
                enable_automatic_maintenance();
                const WCHAR *message_after_enable = LANG_TEXT(
                    L"Automatic maintenance has been \"enabled\".",
                    L"自動メンテナンスを「有効」に設定しました。");
                MessageBoxW(NULL, message_after_enable, WINDOW_TITLE, MB_OK | MB_ICONINFORMATION);
            }
        } else {
            const WCHAR *message_before_disable = LANG_TEXT(
                L"Automatic maintenance is \"enabled\".\nDo you want to disable it?",
                L"自動メンテナンスは「有効」に設定されています。\n無効にしますか？");
            int selection = MessageBoxW(NULL, message_before_disable, WINDOW_TITLE, MB_YESNO | MB_ICONQUESTION);
            if (selection == IDYES) {
                disable_automatic_maintenance();
                const WCHAR *message_after_disable = LANG_TEXT(
                    L"Automatic maintenance has been \"disabled\".",
                    L"自動メンテナンスを「無効」に設定しました。");
                MessageBoxW(NULL, message_after_disable, WINDOW_TITLE, MB_OK | MB_ICONINFORMATION);
            }
        }
    } catch (WSTRING error) {
        MessageBoxW(NULL, error.c_str(), WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        const WCHAR *message = LANG_TEXT(
            L"An unknown error has occurred.",
            L"不明なエラーが発生しました。");
        MessageBoxW(NULL, message, WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}

bool is_automatic_maintenance_disabled() {
    bool is_automatic_maintenance_disabled = false;
    HKEY registry_handle = {};
    LSTATUS open_result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, SUB_KEY, 0, KEY_READ | KEY_WOW64_64KEY, &registry_handle);
    if (open_result != ERROR_SUCCESS) {
        WSTRING error_msg = LANG_TEXT(
            L"Failed to open registry.",
            L"レジストリキーの参照に失敗しました。");
        WSTREAM error_id;
        error_id << L"(" << open_result << L")";
        RegCloseKey(registry_handle);
        throw error_msg.append(error_id.str());
    }
    DWORD value_type = REG_NONE;
    DWORD value_data = 0;
    DWORD value_data_size = sizeof(DWORD);
    LSTATUS read_result = RegQueryValueExW(registry_handle,
        VALUE_NAME, NULL, &value_type, reinterpret_cast<BYTE *>(&value_data), &value_data_size);
    if (read_result == ERROR_FILE_NOT_FOUND) {
        RegCloseKey(registry_handle);
        return false;
    } else if (read_result == ERROR_MORE_DATA) {
        WSTRING error_msg = LANG_TEXT(
            L"The registry value is too large.",
            L"レジストリ値のサイズが大きすぎます。");
        WSTREAM error_id;
        error_id << L"(" << read_result << L")";
        RegCloseKey(registry_handle);
        throw error_msg.append(error_id.str());
    } else if (read_result != ERROR_SUCCESS) {
        WSTRING error_msg = LANG_TEXT(
            L"Failed to query regstry value.",
            L"レジストリ値の参照に失敗しました。");
        WSTREAM error_id;
        error_id << L"(" << read_result << L")";
        RegCloseKey(registry_handle);
        throw error_msg.append(error_id.str());
    }
    RegCloseKey(registry_handle);
    return (value_data == 1);
}

void enable_automatic_maintenance() {
    HKEY registry_handle = {};
    LSTATUS open_result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, SUB_KEY, 0, KEY_WRITE | KEY_WOW64_64KEY, &registry_handle);
    if (open_result != ERROR_SUCCESS) {
        WSTRING error_msg = LANG_TEXT(
            L"Failed to open registry.",
            L"レジストリキーの参照に失敗しました。");
        WSTREAM error_id;
        error_id << L"(" << open_result << L")";
        RegCloseKey(registry_handle);
        throw error_msg.append(error_id.str());
    }
    LSTATUS delete_value_result = RegDeleteValueW(registry_handle, VALUE_NAME);
    if (delete_value_result != ERROR_SUCCESS) {
        WSTRING error_msg = LANG_TEXT(
            L"Failed to delete registry value.",
            L"レジストリ値の削除に失敗しました。");
        WSTREAM error_id;
        error_id << L"(" << delete_value_result << L")";
        throw error_msg.append(error_id.str());
    }
    RegCloseKey(registry_handle);
    return;
}

void disable_automatic_maintenance() {
    HKEY registry_handle = {};
    LSTATUS open_result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, SUB_KEY, 0, KEY_WRITE | KEY_WOW64_64KEY, &registry_handle);
    if (open_result != ERROR_SUCCESS) {
        WSTRING error_msg = LANG_TEXT(
            L"Failed to open registry.",
            L"レジストリキーの参照に失敗しました。");
        WSTREAM error_id;
        error_id << L"(" << open_result << L")";
        RegCloseKey(registry_handle);
        throw error_msg.append(error_id.str());
    }
    DWORD value_data = 1;
    LSTATUS write_value_result = RegSetValueExW(registry_handle,
        VALUE_NAME, 0, REG_DWORD, reinterpret_cast<BYTE *>(&value_data), sizeof(value_data));
    if (write_value_result != ERROR_SUCCESS) {
        WSTRING error_msg = LANG_TEXT(
            L"Failed to write registry value.",
            L"レジストリ値の書き込みに失敗しました。");
        WSTREAM error_id;
        error_id << L"(" << write_value_result << L")";
        RegCloseKey(registry_handle);
        throw error_msg.append(error_id.str());
    }
    RegCloseKey(registry_handle);
    return;
}
