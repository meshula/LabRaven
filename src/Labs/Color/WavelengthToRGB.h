//
//  WavelengthToRGB.h
//  LabExcelsior
//
//  Created by Nick Porcino on 5/11/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef WavelengthToRGB_h
#define WavelengthToRGB_h

#include <stdio.h>

#ifdef __cplusplus
extern "C"
#endif
NcXYZ NcCIE1931ColorFromWavelength(float lambda, bool approx);

#endif /* WavelengthToRGB_h */
