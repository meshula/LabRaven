
#ifndef PXR_BASE_GF_NC_NANOCOLOR_UTILS_H
#define PXR_BASE_GF_NC_NANOCOLOR_UTILS_H

#include "nanocolor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NcISO17321ColorChipsAP0      NCCONCAT(NCNAMESPACE, ISO17321ColorChipsAP0)
#define NcISO17321ColorChipsNames    NCCONCAT(NCNAMESPACE, ISO17321ColorChipsNames)
#define NcCheckerColorChipsSRGB      NCCONCAT(NCNAMESPACE, CheckerColorChipsSRGB)
#define NcMcCamy1976ColorChipsYxy    NCCONCAT(NCNAMESPACE, McCamy1976ColorChipsYxy)

/// \brief Returns the names of the 24 color chips in the ISO 17321 color charts.
/// \return An array of const char pointers containing the names. A nullptr
///         follows the last name.
NCAPI const char** NcISO17321ColorChipsNames(void);

/// \brief Returns a pointer to 24 color values in AP0 corresponding to
///        the 24 color chips in ISO 17321-1:2012 Table D.1.
/// \return An array of NcRGB containing the color values.
NCAPI NcRGB* NcISO17321ColorChipsAP0(void);

/// \brief Returns color values under D65 illuminant for the checker color chips,
///        similar but not matching the ISO table.
/// \return An array of NcRGB containing the color values.
NCAPI NcRGB* NcCheckerColorChipsSRGB(void);

/// \brief Returns color values under Illuminant C for the McCamy 1976 color chips,
///        similar to but not matching the ISO table or the x-rite table.
/// \return An array of NcXYZ containing the color values.
NCAPI NcYxy* NcMcCamy1976ColorChipsYxy(void);

#ifdef __cplusplus
}
#endif

#endif /* PXR_BASE_GF_NC_NANOCOLOR_UTILS_H */

