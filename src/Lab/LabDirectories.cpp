
#include "Lab/LabDirectories.h"
#include <string.h>
#include <vector>

#ifdef _WIN32
# include <Windows.h>
# include <ShlObj.h>
# include <direct.h>  // For _mkdir
# define mkdir _mkdir
#elif defined(__APPLE__)
# include <mach-o/dyld.h>
# include <unistd.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <pwd.h>
# include <CoreFoundation/CoreFoundation.h>
# include <iostream>
# include <string>
#elif defined(__linux__)
# include <unistd.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <pwd.h>
# include <limits.h>
#endif

#include <sqlite3.h>
#include <map>
#include <string>
#include <mutex>
#include <iostream>


// https://stackoverflow.com/questions/11238918/s-isreg-macro-undefined Windows
// does not define the S_ISREG and S_ISDIR macros in stat.h, so we do. We have
// to define _CRT_INTERNAL_NONSTDC_NAMES 1 before #including sys/stat.h in
// order for Microsoft's stat.h to define names like S_IFMT, S_IFREG, and
// S_IFDIR, rather than just defining  _S_IFMT, _S_IFREG, and _S_IFDIR as it
// normally does.
#define _CRT_INTERNAL_NONSTDC_NAMES 1
#include <sys/stat.h>
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
  #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
  #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#ifdef _WIN32

extern "C"
const char* lab_application_executable_path(const char * argv0)
{
    static char buf[1024] = { 0 };
    if (!buf[0]) {
        DWORD ret = GetModuleFileNameA(NULL, buf, sizeof(buf));
        if (ret == 0 && argv0)
            strncpy(buf, argv0, sizeof(buf));
    }

    return buf;
}

extern "C"
const char* lab_application_preferences_path(const char* fallbackName) {
    static std::string path;

    // Get the user's home directory
    WCHAR wszPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, wszPath))) {
        // Convert WCHAR to char
        char szPath[MAX_PATH];
        if (WideCharToMultiByte(CP_UTF8, 0, wszPath, -1, szPath, sizeof(szPath), NULL, NULL)) {
            path.assign(szPath);
        } else {
            // Handle conversion error
            std::cerr << "Error converting WCHAR to char." << std::endl;
            return nullptr;
        }
    } else {
        std::cerr << "Error fetching user's home directory." << std::endl;
        return nullptr;
    }

    path += "\\AppData\\Roaming\\"; // Windows equivalent of ~/Library/Preferences/

    // Get the module file name (executable path)
    CHAR szModuleName[MAX_PATH];
    if (GetModuleFileNameA(NULL, szModuleName, MAX_PATH)) {
        // Extract the filename without extension
        std::string moduleName = szModuleName;
        size_t lastSlash = moduleName.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            moduleName = moduleName.substr(lastSlash + 1);
            size_t lastDot = moduleName.find_last_of(".");
            if (lastDot != std::string::npos) {
                moduleName = moduleName.substr(0, lastDot);
            }
        }
        path += moduleName;
    } else {
        path += fallbackName;
    }

    path += "\\";
    
    // Create the directory if it doesn't exist
    if (_mkdir(path.c_str()) != 0 && errno != EEXIST) {
        char errorStr[256];
        strerror_s(errorStr, sizeof(errorStr), errno);
        std::cerr << "Error creating directory: " << errorStr << std::endl;
    }

    return path.c_str();
}

#elif defined(__APPLE__)

extern "C"
const char* lab_application_executable_path(const char * argv0)
{
    static char buf[1024] = { 0 };
    if (!buf[0]) {
        uint32_t size = sizeof(buf);
        int ret = _NSGetExecutablePath(buf, &size);
        if (0 == ret && argv0)
            strncpy(buf, argv0, sizeof(buf));
    }
    return buf;
}

extern "C"
const char* lab_application_preferences_path(const char* fallbackName) {
    static std::string path;
    if (path.size() > 0)
        return path.c_str();
    
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw != nullptr) {
        path.assign(pw->pw_dir);
    } else {
        const char* homeEnv = getenv("HOME");
        if (homeEnv) {
            path.assign(homeEnv);
        } else {
            std::cerr << "Error: Unable to determine home directory." << std::endl;
            return nullptr;
        }
    }

    path += "/Library/Preferences/";

    // Get the bundle identifier
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFStringRef bundleIdentifier = CFBundleGetIdentifier(mainBundle);

    char bundleIdentifierCString[256] = {0};
    if (bundleIdentifier) {
        // Use CFStringGetCString for a reliable conversion
        if (!CFStringGetCString(bundleIdentifier, bundleIdentifierCString, sizeof(bundleIdentifierCString), kCFStringEncodingUTF8)) {
            std::cerr << "Error: Unable to convert bundle identifier to C string." << std::endl;
        }
    }

    if (bundleIdentifierCString[0] != '\0') {
        path += bundleIdentifierCString;
        printf("Bundle Identifier: %s\n", bundleIdentifierCString);
    } else if (fallbackName) {
        path += fallbackName;
    } else {
        std::cerr << "Error: No valid bundle identifier or fallback name provided." << std::endl;
        return nullptr;
    }

    path += "/";

    // Create the directory if it doesn't exist
    if (mkdir(path.c_str(), 0755) != 0 && errno != EEXIST) {
        std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
        return nullptr;
    }

    return path.c_str();
}

extern "C"
const char* lab_application_resource_path(const char* argv0, const char* rsrc) {
    static std::string path;
    
    // Get the main bundle
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    
    // Get the URL for the bundle
    CFURLRef bundleURL = CFBundleCopyBundleURL(mainBundle);

    // Get the URL for the Resources directory
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);

    // Concatenate bundleURL and resourcesURL to get the full URL
    CFURLRef fullURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, bundleURL, CFURLGetString(resourcesURL), true);

    // Get the absolute file path from the URL
    CFStringRef fullPath = CFURLCopyFileSystemPath(fullURL, kCFURLPOSIXPathStyle);
    const char *fullPathCString = CFStringGetCStringPtr(fullPath, kCFStringEncodingUTF8);

    // Print or use the absolute file path
    if (fullPathCString) {
        path.assign(fullPathCString);
    } else {
        fprintf(stderr, "Error getting Resources directory path.\n");
    }
    
    // Release the allocated resources
    // Release the allocated resources
    CFRelease(bundleURL);
    CFRelease(resourcesURL);
    CFRelease(fullURL);
    CFRelease(fullPath);
    
    return path.c_str();
}

#elif defined(__linux__)

extern "C"
const char* lab_application_executable_path(const char * argv0)
{
    static char buf[1024] = { 0 };
    if (!buf[0]) {
        ssize_t size = readlink("/proc/self/exe", buf, sizeof(buf));
        if (size == 0)
            strncpy(buf, argv0, sizeof(buf));
    }
    return buf;
}

extern "C"
const char* lab_application_preferences_path(const char* fallbackName) {
    static std::string path;

    // Get the user's home directory
    const char* homeDir = getenv("HOME");
    if (homeDir != nullptr) {
        path.assign(homeDir);
    } else {
        std::cerr << "Error fetching user's home directory." << std::endl;
        return nullptr;
    }

    path += "/.config/"; // Linux equivalent of ~/Library/Preferences/

    // Get the module file name (executable path)
    char executablePath[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", executablePath, sizeof(executablePath) - 1);
    if (count != -1) {
        executablePath[count] = '\0';
        // Extract the filename without extension
        std::string moduleName = basename(executablePath);
        size_t lastDot = moduleName.find_last_of(".");
        if (lastDot != std::string::npos) {
            moduleName = moduleName.substr(0, lastDot);
        }
        path += moduleName;
    } else {
        path += fallbackName;
    }

    path += "/";

    if (mkdir(path.c_str(), 0777) != 0 && errno != EEXIST) {
        std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
    }
    
    return path.c_str();
}

#else
# error Unknown platform
#endif

static void remove_filename(char* fn)
{
    if (!fn)
        return;

    char* sep = strrchr(fn, '\\');
    if (sep) {
        ++sep;
        *sep = '\0';
        return;
    }
    sep = strrchr(fn, '/');
    if (sep) {
        ++sep;
        *sep = '\0';
    }
}

static void remove_sep(char* buf)
{
    if (!buf)
        return;
    int sz = (int) strlen(buf);
    if (sz > 0 && (buf[sz - 1] == '/' || buf[sz - 1] == '\\'))
        buf[sz - 1] = '\0';
}

static void add_filename(char* buf, const char* fn) {
    if (!buf || !fn)
        return;
    int sz = (int) strlen(buf);
    if (sz > 0 && (buf[sz - 1] != '/' && buf[sz - 1] != '\\')) {
        buf[sz] = '/';
        buf[sz+1] = '\0';
    }
    strcat(buf, fn);
}

static void normalize_path(char* buf) {
    if (!buf)
        return;
    int sz = (int) strlen(buf);
    for (int i = 0; i < sz; ++i)
        if (buf[i] == '\\')
            buf[i] = '/';
}


extern "C"
bool lab_create_path(const char* path) {
    std::string fullPath(path);

    // Iterate through the path components
    size_t pos = 0;
    while ((pos = fullPath.find_first_of("/\\", pos)) != std::string::npos) {
        std::string subPath = fullPath.substr(0, pos + 1);

        // Create the directory
#ifdef _WIN32
        if (_mkdir(subPath.c_str()) != 0) {
#else
            if (mkdir(subPath.c_str(), 0777) != 0) {
#endif
                // Directory creation failed
                std::cerr << "Error creating directory: " << subPath << std::endl;
                return false;
            }

            // Move to the next component
            ++pos;
        }

        return true;
    }

#if !defined(__APPLE__)
    extern "C"
    const char* lab_application_resource_path(const char * argv0, const char* rsrc)
    {
        static char* buf = nullptr;
        if (buf || !argv0 || !rsrc)
            return buf;

        // buf can only be shorter than this.
        buf = (char*) malloc(strlen(argv0) + strlen(rsrc) + 3 + strlen("install"));

        std::vector<char> checkVec;
        checkVec.resize(strlen(argv0) + 1, 0);
        char* check = checkVec.data();
        strncpy(check, argv0, strlen(argv0));

        int sz = 0;
        do {
            remove_sep(check);
            sz = (int) strlen(check);
            remove_filename(check);
            int new_sz = (int) strlen(check);
            if (new_sz == sz)
                return nullptr;
            sz = new_sz;

            strncpy(buf, check, strlen(check)+1);
            add_filename(buf, rsrc);
            normalize_path(buf);
            struct stat sb;
            int res = stat(buf, &sb);
            if (res == 0 && S_ISDIR(sb.st_mode)) {
                return buf;
            }

            strncpy(buf, check, strlen(check)+1);
            add_filename(buf, "install");
            add_filename(buf, rsrc);
            normalize_path(buf);
            res = stat(buf, &sb);
            if (res == 0 && S_ISDIR(sb.st_mode)) {
                return buf;
            }

        } while (true);
    }
#endif

namespace {
    std::map<std::string, std::string> prefs;
    std::mutex prefsMutex;
    std::once_flag initFlag;
    sqlite3* db = nullptr;
}

LabPreferences LabPreferencesLock() {
    return LabPreferences(prefs, prefsMutex);
}

// Utility function to get the full path to the preferences database file
const char* lab_application_preferences_path(const char* fallbackName);

// Function to initialize the SQLite database and load preferences into cache
static void initializeDatabase() {
    const char* path = lab_application_preferences_path("preferences.db");
    if (!path) {
        std::cerr << "Error: Could not determine preferences path." << std::endl;
        return;
    }

    if (sqlite3_open(path, &db) != SQLITE_OK) {
        std::cerr << "Error: Could not open SQLite database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Create the preferences table if it doesn't exist
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS preferences (
            key TEXT PRIMARY KEY,
            value TEXT
        );
    )";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error: Could not create table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return;
    }

    // lock the mutex for the preferences map
    std::unique_lock<std::mutex> lock(prefsMutex);

    // Load all existing preferences into the in-memory map
    const char* selectAllSQL = "SELECT key, value FROM preferences;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, selectAllSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (key && value) {
                prefs[key] = value;
            }
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Error: Could not load preferences: " << sqlite3_errmsg(db) << std::endl;
    }
}

// Function to set a preference (key-value pair)
extern "C"
void lab_set_pref_for_key(const char* key, const char* value) {
    std::call_once(initFlag, initializeDatabase);
    if (!db) return;

    const char* insertOrUpdateSQL = R"(
        INSERT INTO preferences (key, value) VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value;
    )";

    // lock the mutex for the preferences map
    std::unique_lock<std::mutex> lock(prefsMutex);

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertOrUpdateSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Error: Could not set preference: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_finalize(stmt);

        // Update the in-memory cache
        prefs[key] = value;
    } else {
        std::cerr << "Error: Could not prepare statement: " << sqlite3_errmsg(db) << std::endl;
    }
}

// Function to get a preference (value for a given key)
extern "C"
const char* lab_pref_for_key(const char* key) {
    std::call_once(initFlag, initializeDatabase);

    // lock the mutex for the preferences map
    std::unique_lock<std::mutex> lock(prefsMutex);

    auto it = prefs.find(key);
    if (it != prefs.end()) {
        return it->second.c_str(); // Return a reference to the cached value
    }

    return nullptr; // Key not found
}

// Function to close the SQLite database (optional cleanup)
extern "C"
void lab_close_preferences() {
    if (db) {
        // lock the mutex for the preferences map
        std::unique_lock<std::mutex> lock(prefsMutex);
        sqlite3_close(db);
        db = nullptr;
    }
}

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

bool lab_reveal_on_desktop(const char* path) {
    CFURLRef u;
    int err = 0;

    u = CFURLCreateFromFileSystemRepresentation(NULL,
                                                (const UInt8 *) path,
                                                strlen(path),
                                                false);
    if (u != NULL) {
        err = LSOpenCFURLRef(u, NULL);
        CFRelease(u);
    } else {
        err = 1;
    }

    return err == noErr;
}
#endif

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>

bool lab_reveal_on_desktop(const char* path) {
    wchar_t wPath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wPath, MAX_PATH);

    if (PathIsDirectoryW(wPath)) {
        return (int)ShellExecuteW(NULL, L"open", wPath, NULL, NULL, SW_SHOW) > 32;
    } else {
        ITEMIDLIST* pidl = ILCreateFromPathW(wPath);
        if (pidl) {
            SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
            ILFree(pidl);
            return true;
        }
    }
    return false;
}
#endif

#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <string>

static bool is_directory(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool lab_reveal_on_desktop(const char* path) {
    if (fork() == 0) {
        const char* args[3] = {"xdg-open", path, NULL};
        execvp(args[0], const_cast<char* const*>(args));
        _exit(1); // Only reached if execvp fails
    }
    return true;
}
#endif

