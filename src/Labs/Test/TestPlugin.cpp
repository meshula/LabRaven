
#include "Labs/Plugin/cr.h"

// set up PLUGIN_API with declspec or attribute per platform
#if defined(_WIN32)
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

// a single exported test function that returns the name of the plugin.
extern "C" PLUGIN_API const char* PluginName() {
    return "TestPlugin!";
}

// This interface is mandated by the cr.h library.
extern "C" PLUGIN_API int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
    switch (operation) {
        case CR_LOAD:   // loading back from a reload
        case CR_UNLOAD: // preparing to a new reload
        case CR_CLOSE:  // the plugin will close and not reload anymore
        case CR_STEP:   // the plugin will do a step of work
            return CR_NONE; // no error to report
        default:
            return CR_OTHER; // unknown operation
    }
}
