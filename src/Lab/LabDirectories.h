
#ifndef LAB_DIRECTORIES_H
#define LAB_DIRECTORIES_H
#ifdef __cplusplus
extern "C" {
#endif

const char* lab_temp_directory_path();

// get the path to the executable, including the executable name
const char* lab_application_executable_path(const char* argv0);

// get the path to the application's resources directory
const char* lab_application_resource_path(const char* argv0, const char* root);

// get the path to the application's preferences directory
const char* lab_application_preferences_path(const char* fallbackName);

// create a directory at the given path, including any necessary parent directories
bool lab_create_path(const char* path);

// reveal a file or folder in the desktop file manager
bool lab_reveal_on_desktop(const char* path);

// these functions allow storing and retrieving preferences as key-value pairs
void lab_set_pref_for_key(const char* key, const char* value);
const char* lab_pref_for_key(const char* key);
void lab_close_preferences(); // call as preparation for application exit

const char* lab_temp_directory_path();

#ifdef __cplusplus
}

#include <map>
#include <mutex>
#include <string>

// RAII class to lock the preferences map and provide access to it
// This class is used to ensure that the preferences map is locked
// when it is accessed, and that the lock is released when the
// LabPreferences object goes out of scope.
class LabPreferences {
public:
    const std::map<std::string, std::string>& prefs;
    // lock guard for the preferences map
    std::unique_lock<std::mutex> lock;

    // construct with a reference to the prefs, and a lock on the mutex
    LabPreferences(std::map<std::string, std::string>& prefs, std::mutex& mutex)
        : prefs(prefs), lock(mutex) {}

    ~LabPreferences() {
        lock.unlock();
    }
};

// Take a lock on the preferences map and return a LabPreferences object.
// This function is used to ensure that the preferences map is locked
// when it is accessed, and that the lock is released when the
// LabPreferences object goes out of scope. Do not hold onto the
// LabPreferences object for longer than necessary.
LabPreferences LabPreferencesLock();

#endif

#endif
