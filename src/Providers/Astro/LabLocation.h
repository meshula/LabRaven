//
//  LabLocation.h
//  LabExcelsior
//
//  Created by Nick Porcino on 3/29/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef Header_h
#define Header_h

#include <time.h>

namespace  lab {
bool GetLocation(float& latitude, float& longitude);
tm LocalTimeFromUTC(tm);
}

#endif /* Header_h */
