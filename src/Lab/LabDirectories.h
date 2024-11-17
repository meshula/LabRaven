
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
#ifdef __cplusplus
}
#endif

#endif
