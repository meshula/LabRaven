
// set up PLUGIN_API with declspec or attribute per platform
#if defined(_WIN32)
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

// a single exported test function that returns the name of the plugin.
extern "C" PLUGIN_API const char* PluginName() {
    return "TestPluginx";
}
