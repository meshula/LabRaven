#include <stddef.h>
#include <stdbool.h>

typedef struct LabViewDimensions {
    float w, h;             // view full width and height
    float wx, wy, ww, wh;   // window within the view
} LabViewDimensions;

typedef struct LabViewInteraction {
    LabViewDimensions view;
    // = 0
    float x, y , dt;
    // false
    bool start, end;  // start and end of a drag
} LabViewInteraction;

// Define ModeActivities in a C-compatible way
typedef struct LabActivity {
    // nullptr
    void (*Activate)(void*) ;
    void (*Deactivate)(void*) ;
    void (*Update)(void*) ;
    void (*Render)(void*, const LabViewInteraction*) ;
    void (*RunUI)(void*, const LabViewInteraction*) ;
    void (*Menu)(void*) ;
    void (*ToolBar)(void*) ;
    int  (*ViewportHoverBid)(void*, const LabViewInteraction*) ;
    void (*ViewportHovering)(void*, const LabViewInteraction*) ;
    int  (*ViewportDragBid)(void*, const LabViewInteraction*) ;
    void (*ViewportDragging)(void*, const LabViewInteraction*) ;
    const char* name ; // string is not owned by the activity
    bool active ;
} LabActivity;
