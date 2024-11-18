
#ifndef LAB_DIRECTORIES_H
#define LAB_DIRECTORIES_H
#ifdef __cplusplus
extern "C" {
#endif
const char* lab_application_executable_path(const char* argv0);
const char* lab_application_resource_path(const char* argv0, const char* root);
const char* lab_application_preferences_path(const char* fallbackName);
bool lab_create_path(const char* path);

void lab_set_pref_for_key(const char* key, const char* value);
const char* lab_pref_for_key(const char* key);
void lab_close_preferences(); // call as preparation for application exit

#ifdef __cplusplus
}

#include <map>
#include <mutex>
#include <string>

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

LabPreferences LabPreferencesLock();

#endif

#endif
