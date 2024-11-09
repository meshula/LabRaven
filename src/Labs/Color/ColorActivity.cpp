//
//  ColorActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/6/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ColorActivity.hpp"

//#include "LabMath.h"
//#include "metalRay.h"
#include "nanocolorUtils.h"
#include "imgui.h"
//#include "LabGL.h"
//#include "lab_imgui_ext.hpp"
#include "implot.h"
#include "WavelengthToRGB.h"
#include "UIElements.hpp"

namespace lab {

struct XY { float x; float y; };

struct LabV3f {
    float x, y, z;
};

struct CMFMatch {
    double wavelength, x, y, z;
};


struct ColorActivity::data {
    data() {
        working_cs = "srgb_texture";//lin_ap1";
        
        const char** nanocolor_names = NcRegisteredColorSpaceNames();
        cs_index = 0;
        const char** names = nanocolor_names;
        while (*names != nullptr) {
            if (!strcmp(working_cs.c_str(), *names)) {
                break;
            }
            ++cs_index;
            ++names;
        }
    }

    bool ui_visible = false;
    bool horseshoe_visible = true;
    bool macbeth_visible = true;
    bool fill_chart = false;
    bool planckian_visible = true;
    bool show_gamuts = true;
    
    int horseshoe_points = 0;
    int stored_horseshoe_points = 0;
    float horseshow_sz = 0.f;
    XY horseshoe[1024];
    
    int RenderColorChart(const LabViewDimensions& d);

    void PlotSpectralLocus(const CMFMatch* cmfData, int dataSize);
    
    int cs_index = 0;
    std::string working_cs;
};

namespace {
// ISO 17321-1:2012 Table D.1
// ap0 is the aces name for 2065-1
/*
 CIE 1931
 AP0: ACES 2065-1             White Point  AP1: cg, cc, cct, proxy
 red     green   blue                 red    green  blue
 x        0.7347  0.0000  0.0001    0.32168    0.713  0.165  0.128
 y        0.2653  1.0000 -0.0770    0.33767    0.293  0.830  0.044
 */

static LabV3f ISO17321_r709[24] = {
    { 0.18326745f, 0.08043892f, 0.048751256f },
    { 0.57313635f, 0.30258403f, 0.202327929f },
    { 0.12166137f, 0.19800845f, 0.304997339f },
    { 0.10219760f, 0.14818909f, 0.048250962f },
    { 0.24686856f, 0.22365069f, 0.401356396f },
    { 0.14953951f, 0.50350513f, 0.364782663f },
    { 0.71046856f, 0.19796405f, 0.022039876f },
    { 0.07562225f, 0.10815631f, 0.342598452f },
    { 0.56877221f, 0.09196745f, 0.112304507f },
    { 0.11414032f, 0.04817642f, 0.135181963f },
    { 0.36200031f, 0.48945945f, 0.043899454f },
    { 0.80256076f, 0.35834120f, 0.024296749f },
    { 0.03318762f, 0.05190392f, 0.282282677f },
    { 0.06996592f, 0.29976133f, 0.057260607f },
    { 0.45702324f, 0.03133905f, 0.041221802f },
    { 0.86766099f, 0.56453118f, 0.008749157f },
    { 0.51730194f, 0.09086778f, 0.269164551f },
    { 0.f        , 0.24431530f, 0.351227007f }, // out of gamut r709 r: -0.02131250
    { 0.90906175f, 0.86448085f, 0.783917811f },
    { 0.60242857f, 0.56923897f, 0.523116990f },
    { 0.37003347f, 0.35136621f, 0.324028232f },
    { 0.21204851f, 0.20125644f, 0.185761815f },
    { 0.09786984f, 0.09471846f, 0.088412194f },
    { 0.03839120f, 0.03739227f, 0.035869006f },
};


/*
    This is actually u'v', u'v' is uv scaled by 1.5 along the v axis
*/

typedef struct {
    float Y;
    float u;
    float v;
} NcYuvPrime;

NcYuvPrime _NcChromaticityToNcYuvPrime(NcChromaticity cr) {
    return (NcYuvPrime) {
        (4.f * cr.x) / ((-2.f * cr.x) + (12.f * cr.y) + 3.f),
        (9.f * cr.y) / ((-2.f * cr.x) + (12.f * cr.y) + 3.f)
    };
}


static XY cieplot(NcChromaticity c_,
                  float sz, float xoff, float yoff, float height) {

    //NcYuvPrime c = _NcChromaticityToNcYuvPrime(c_);
    float cx = c_.x;
    float cy = c_.y;

    float sx = xoff + cx * sz;
    float sy = yoff + height - cy * sz;
    return (XY) { sx, sy };
}

int isLeft(XY a, XY b, XY c) {
    return ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) > 0;
}

int windingNumber(XY point, XY polygon[], int n) {
    int windingNumber = 0;
    for (int i = 0; i < n; i++) {
        XY current = polygon[i];
        XY next = polygon[(i + 1) % n];
        
        if (current.y <= point.y) {
            if (next.y > point.y && isLeft(current, next, point)) {
                windingNumber++;
            }
        } else {
            if (next.y <= point.y && isLeft(next, current, point)) {
                windingNumber--;
            }
        }
    }
    
    return windingNumber;
}


static inline float sign_of(float x) {
    return x > 0 ? 1.f : (x < 0) ? -1.f : 0.f;
}
} // anon

XY magook = {1,1};

int ColorActivity::data::RenderColorChart(const LabViewDimensions& d) {
    if (d.ww <= 0 || d.wh <= 0)
        return 0; // not initialized yet

    static bool once = true;
    if (once) {
        once = false;
        for (float T = 1000; T <= 2000; T += 100) {
            NcYxy result = NcKelvinToYxy(T, 1);
            printf("%f: NcYxy(Y=%f, x=%f, y=%f)\n", T, result.Y, result.x, result.y);
        }
    }

    const NcColorSpace* lin_ap0 = NcGetNamedColorSpace("lin_ap0");
    const NcColorSpace* lin_rec2020 = NcGetNamedColorSpace("lin_rec2020");
    const NcColorSpace* lin_rec709 = NcGetNamedColorSpace("lin_rec709");
    const NcColorSpace* lin_dp3 = NcGetNamedColorSpace("lin_displayp3");
    const NcColorSpace* display = lin_dp3;
    const NcColorSpace* srgb = NcGetNamedColorSpace("sRGB");
    const NcColorSpace* identity_cs = NcGetNamedColorSpace("identity");
    /*
    const NcColorSpace* g22_rec709 = NcGetNamedColorSpace("g22_rec709");
     NcXYZ c1 = NcKelvinToXYZ(6666.f, 1.f);
    NcRGB rgb1 = NcRGBFromYxy(lin_rec2020, c1); // 0.9651179, 0.9388702, 1
    NcRGB rgb2 = NcRGBFromYxy(lin_rec709, c1);  // 0.9714133, 0.928792, 1
     NcXYZ c2 = NcKelvinToXYZ(3000.f, 1.f);
    NcRGB rgb3 = NcRGBFromYxy(g22_rec709, c1);  // 1, 0.4767293, 0.1536329
    */
    int height = d.wh /2;
    int xoff = d.wx / 2 + 20;
    int yoff = d.wy / 2 - 60;
    const float sz = height * 0.8f;
    
#if 0

    BeginDrawing();
    SetTexture(0);
    
    XY a, b;
    
    // plot the axes
    
    for (float xf = 0.1f; xf <= 0.9f; xf += 0.1f) {
        a = cieplot({xf,0}, sz, xoff, yoff, height);
        b = cieplot({xf,0.9f}, sz, xoff, yoff, height);
        DrawLineEx(a.x, a.y, b.x, b.y, 1.5f, (Color) {180,180,180,255});
    }
    for (float xf = 0.1f; xf <= 1.0f; xf += 0.1f) {
        a = cieplot({0,xf}, sz, xoff, yoff, height);
        b = cieplot({0.8f,xf}, sz, xoff, yoff, height);
        DrawLineEx(a.x, a.y, b.x, b.y, 1.5f, (Color) {180,180,180,255});
    }
    
    a = cieplot({0,0}, sz, xoff, yoff, height);
    b = cieplot({0.8f,0}, sz, xoff, yoff, height);
    DrawLineEx(a.x, a.y, b.x, b.y, 1.5f, BLACK);
    DrawCircle(a.x, a.y, 4, BLACK);
    
    /*
     ___ ___  ___  _ ____ _______ _    ___ _    _
    |_ _/ __|/ _ \/ |__  |__ /_  ) |  / __| |_ (_)_ __ ___
     | |\__ \ (_) | | / / |_ \/ /| | | (__| ' \| | '_ (_-<
    |___|___/\___/|_|/_/ |___/___|_|  \___|_||_|_| .__/__/
                                                 |_|
     */

    // plot the Macbeth chart
    if (macbeth_visible) {
        static bool print_debug_values = false;
        
        NcRGB* ISO17321_ap0 = NcISO17321ColorChipsAP0();
        NcRGB* ISO17321_srgb = NcCheckerColorChipsSRGB();
        XY upper_left = cieplot({1,1}, sz, xoff, yoff, height);
        upper_left.x -= 80;
        upper_left.y += 52;
        const float chipSz = 40;
        for (int i = 0; i < 24; ++i) {
            // outer rectangle
            NcRGB ap0rgb = {ISO17321_ap0[i].r, ISO17321_ap0[i].g, ISO17321_ap0[i].b};
            NcRGB display_rgb1 = NcTransformColor(srgb, display, ISO17321_srgb[i]);
            static bool applyPow = false;
            if (applyPow) {
                display_rgb1.r = powf(display_rgb1.r, 1.f/2.2f);
                display_rgb1.g = powf(display_rgb1.g, 1.f/2.2f);
                display_rgb1.b = powf(display_rgb1.b, 1.f/2.2f);
            }
            Color c1 = {
                (unsigned char)(display_rgb1.r * 255.f),
                (unsigned char)(display_rgb1.g * 255.f),
                (unsigned char)(display_rgb1.b * 255.f),
                255
            };
                        
            float x = (upper_left.x + (i % 6) * chipSz);
            float y = (upper_left.y + (i / 6) * chipSz);
            XY xy = { x, y };
            
            DrawRectangle(xy.x, xy.y, chipSz, chipSz, c1);

            // inner rectangle
            NcRGB display_rgb2 = NcTransformColor(display, lin_ap0, ap0rgb);
            
            Color c2 = {
                (unsigned char)(display_rgb2.r * 255.f),
                (unsigned char)(display_rgb2.g * 255.f),
                (unsigned char)(display_rgb2.b * 255.f),
                255
            };

            xy.x += chipSz / 4;
            xy.y += chipSz / 4;

            DrawRectangle(xy.x, xy.y, chipSz /2, chipSz/2, c2);
        }
        print_debug_values = false;
    }
    
   /* ___ ___  ___  _ ____ _______ _    ___ ___ ___ _____  ____   __
     |_ _/ __|/ _ \/ |__  |__ /_  ) |  / __|_ _| __|_ _\ \/ /\ \ / /
      | |\__ \ (_) | | / / |_ \/ /| | | (__ | || _| | | >  <  \ V /
     |___|___/\___/|_|/_/ |___/___|_|  \___|___|___|___/_/\_\  |_| */

    NcRGB* ISO17321_ap0 = NcISO17321ColorChipsAP0();
    
    if (macbeth_visible) {
        // put the chart on the horseshoe
        for (int i = 0; i < 24; ++i) {
            NcRGB ap0rgb = {ISO17321_ap0[i].r, ISO17321_ap0[i].g, ISO17321_ap0[i].b};
            //NcRGB c709rgb = {ISO17321_r709[i].x, ISO17321_r709[i].y, ISO17321_r709[i].z};
            

            NcRGB rgb = NcTransformColor(display, lin_ap0, ap0rgb);
            
            Color color = {
                (unsigned char)(rgb.r * 255.f),
                (unsigned char)(rgb.g * 255.f),
                (unsigned char)(rgb.b * 255.f),
                255
            };
            NcXYZ xyz = NcRGBToXYZ(lin_ap0, ap0rgb);
            NcYxy cr = NcXYZToYxy(xyz);
            XY xy = cieplot((NcChromaticity) {cr.x, cr.y},
                            sz, xoff, yoff, height);
            DrawCircle(xy.x, xy.y, 8, color);
        }
    }
    
    const float lstart = 360.f;
    const float lend = 730.f;
    
    NcXYZ cstart = NcCIE1931ColorFromWavelength(lstart, false);
    NcYxy crStart = NcXYZToYxy(cstart);
    NcXYZ cend = NcCIE1931ColorFromWavelength(lend, false);
    NcYxy crEnd = NcXYZToYxy(cend);

    if (horseshow_sz != sz || stored_horseshoe_points == 0) {
        // compute the horseshoe
        horseshoe_points = 0;
        for (int approx = 0; approx <= 0 /*1*/; ++approx) {
            for (float l = lstart + 1.f; l <= lend; l += 1.f) {
                NcXYZ c = NcCIE1931ColorFromWavelength(l, false);
                NcYxy cr = NcXYZToYxy(c);
                XY xy = cieplot({cr.x, cr.y}, sz, 0, yoff, height);
                XY sxy = { xy.x, xy.y - height };
                horseshoe[horseshoe_points++] = sxy;
            }
        }
        horseshow_sz = sz;
        stored_horseshoe_points = horseshoe_points;
    }

/*
 ___ _ _ _
| __(_) | |
| _|| | | |
|_| |_|_|_|
 */

    if (fill_chart) {
        // fill in the solid wodge of color
        float wodge = 4.f;
        float step = wodge / height;
        for (float x = -step; x <= 0.8f; x += step) {
            for (float y = 0; y <= 0.85f; y += step) {
                NcChromaticity cr = { x, y };
                XY p = cieplot(cr, sz, 0, 0, height);

                XY testP = p;
                testP.y += yoff - height;
                if (windingNumber(testP, horseshoe, horseshoe_points) == 0)
                    continue;

                NcRGB rgb;
                static bool testIdentityTransform = false;
                if (testIdentityTransform) {
                    NcRGB c = {cr.x/cr.y, 1.f, (1.f - cr.x - cr.y) / cr.y};
                    rgb = NcTransformColor(display, identity_cs, c);

                    NcRGB magRgb = {
                        fabsf(rgb.r),
                        fabsf(rgb.g),
                        fabsf(rgb.b) };

                    float maxc = (magRgb.r > magRgb.g) ? magRgb.r : magRgb.g;
                    maxc = maxc > magRgb.b ? maxc : magRgb.b;
                    NcRGB ret = (NcRGB) {
                        sign_of(rgb.r) * rgb.r / maxc,
                        sign_of(rgb.g) * rgb.g / maxc,
                        sign_of(rgb.b) * rgb.b / maxc };
                    rgb = ret;
                }
                else {
                    rgb = NcYxyToRGB(lin_rec2020, {1, cr.x, cr.y});
                }

                Color col = {
                    (unsigned char)(rgb.r * 255.f),
                    (unsigned char)(rgb.g * 255.f),
                    (unsigned char)(rgb.b * 255.f),
                    255
                };
                const float radius = 2.5f;
                DrawRectangle(p.x + xoff - radius, p.y + yoff - radius, radius *2, radius*2, col);
            }
        }
    }

/*
 ___              _            _   _
/ __|_ __  ___ __| |_ _ _ __ _| | | |   ___  __ _  _ ___
\__ \ '_ \/ -_) _|  _| '_/ _` | | | |__/ _ \/ _| || (_-<
|___/ .__/\___\__|\__|_| \__,_|_| |____\___/\__|\_,_/__/
    |_|
 */

    // plot the horseshoe
    for (int approx = 0; approx <= 0 /*1*/; ++approx) {
        NcYxy prev = crStart;
        for (float l = lstart + 1.f; l <= lend; l += 1.f) {
            NcXYZ c = NcCIE1931ColorFromWavelength(l, false);

            NcYxy col = {1, c.x, c.y};
            NcRGB rgb = NcYxyToRGB(lin_rec2020, col);
            Color color = {
                (unsigned char)(rgb.r * 255.f),
                (unsigned char)(rgb.g * 255.f),
                (unsigned char)(rgb.b * 255.f),
                255
            };
            
            XY xy = cieplot({col.x, col.y}, sz, xoff, yoff, height);

            float sx = xy.x;
            float sy = xy.y;
            XY xy2 = cieplot({prev.x, prev.y}, sz, xoff, yoff, height);
            float ex = xy2.x;
            float ey = xy2.y;
            DrawLineEx(sx, sy, ex, ey, 6.f, BLACK);
            DrawLineEx(sx, sy, ex, ey, 2.f, color);

            prev = col;
        }
    }
    
    stored_horseshoe_points = horseshoe_points;
    horseshow_sz = sz;

    a = cieplot({magook.x, magook.y}, sz, xoff, yoff, height);
    DrawCircle(a.x, a.y, 4, BLACK);


/*
 _    _                 __   ___               _
| |  (_)_ _  ___   ___ / _| | _ \_  _ _ _ _ __| |___ ___
| |__| | ' \/ -_) / _ \  _| |  _/ || | '_| '_ \ / -_|_-<
|____|_|_||_\___| \___/_|   |_|  \_,_|_| | .__/_\___/__/
                                         |_|
 */

    // plot the line across the bottom of the horseshoe
    {
        XY xy0 = cieplot({crStart.x, crStart.y}, sz, xoff, yoff, height);
        XY xy1 = cieplot({crEnd.x, crEnd.y}, sz, xoff, yoff, height);
        for (float step = 0.f; step <= 1.f; step += 0.05f) {
            XY start = {xy0.x + (xy1.x - xy0.x) *step,
                xy0.y + (xy1.y - xy0.y)*step };
            
            NcXYZ colxy = {
                cstart.x + (cend.x - cstart.x) * step,
                cstart.y + (cend.y - cstart.y) * step,
                cstart.z + (cend.z - cstart.z) * step,
            };
            
            NcYxy col = {1, colxy.x, colxy.y};
            NcRGB rgb = NcYxyToRGB(lin_rec2020, col);
            Color color = {
                (unsigned char)(rgb.r * 255.f),
                (unsigned char)(rgb.g * 255.f),
                (unsigned char)(rgb.b * 255.f),
                255
            };
            XY end = start;
            end.x += (xy1.x - xy0.x) * 0.05f;
            end.y += (xy1.y - xy0.y) * 0.05f;
            DrawLineEx(start.x, start.y, end.x, end.y, 6.f, BLACK);
            DrawLineEx(start.x, start.y, end.x, end.y, 2.f, color);
        }
    }

/*
  ___                _
 / __|__ _ _ __ _  _| |_ ___
| (_ / _` | '  \ || |  _(_-<
 \___\__,_|_|_|_\_,_|\__/__/
 */

    if (show_gamuts) {
        // plot the gamuts
        static const char* cs_names[] = {
            "lin_ap0",
            "lin_ap1",
            "lin_rec709",
            "lin_rec2020",
            "lin_displayp3"
        };

        for (int i = 0; i < sizeof(cs_names)/sizeof(char*); ++i) {
            const NcColorSpace* cs = NcGetNamedColorSpace(cs_names[i]);
            NcColorSpaceDescriptor desc;
            if (!NcGetColorSpaceDescriptor(cs, &desc))
                continue;

            XY r = cieplot(desc.redPrimary,   sz, xoff, yoff, height);
            XY g = cieplot(desc.greenPrimary, sz, xoff, yoff, height);
            XY b = cieplot(desc.bluePrimary,  sz, xoff, yoff, height);
            DrawLine((int)r.x, (int)r.y, (int)g.x, (int)g.y, BLACK);
            DrawLine((int)r.x, (int)r.y, (int)b.x, (int)b.y, BLACK);
            DrawLine((int)b.x, (int)b.y, (int)g.x, (int)g.y, BLACK);
            
            //XY m = { (r.x + g.x) * 0.5f, (r.y + g.y) * 0.5f };
            XY m = g;
            float length = 50.f;// + i * 20.f;
            g.x += length;
            g.y -= length * 0.5f;
            DrawLineEx((int)m.x, (int)m.y, (int)g.x, (int)g.y, 2, BLACK);

            DrawText({g.x * 2, g.y * 2 - 5}, cs_names[i], 32.f, BLACK);
        }
    }

/*
 ___ _              _   _             _
| _ \ |__ _ _ _  __| |_(_)__ _ _ _   | |   ___  __ _  _ ___
|  _/ / _` | ' \/ _| / / / _` | ' \  | |__/ _ \/ _| || (_-<
|_| |_\__,_|_||_\__|_\_\_\__,_|_||_| |____\___/\__|\_,_/__/
 */

    if (planckian_visible) {
        // Planckian locus
        float prevx = 0, prevy = 0;
        for (float i = 1000.f; i < 15000.f; i += 200.f) {
            NcYxy c = NcKelvinToYxy(i, 1.f);
            if (c.Y < 0.99f)
                continue;
            XY p = cieplot({c.x, c.y}, sz, xoff, yoff, height);

            NcRGB rgb = NcYxyToRGB(lin_rec2020, c);
            
            Color col = {
                (unsigned char)(rgb.r * 255.f),
                (unsigned char)(rgb.g * 255.f),
                (unsigned char)(rgb.b * 255.f),
                255
            };
            if (prevx != 0 && prevy != 0) {
                DrawLineEx(prevx, prevy, p.x, p.y, 6, col);
                DrawLineEx(prevx, prevy, p.x, p.y, 1, BLACK);
            }
            prevx = p.x;
            prevy = p.y;
        }
    }
    
    EndDrawing();
#endif
    return 0;
}


ColorActivity::ColorActivity()
: Activity() {
    _self = new ColorActivity::data;
    
    activity.Render = [](void* instance, const LabViewInteraction* vi) {
        static_cast<ColorActivity*>(instance)->Render(*vi);
    };
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<ColorActivity*>(instance)->RunUI(*vi);
    };
}

ColorActivity::~ColorActivity() {
    delete _self;
}

std::string ColorActivity::GetRenderingColorSpace() const {
    return _self->working_cs;
}

void ColorActivity::SetRenderingColorSpace(const std::string& cs) {
    _self->working_cs = cs;
}

void ColorActivity::Render(const LabViewInteraction& vi)
{
    if (_self->horseshoe_visible)
        _self->RenderColorChart(vi.view);
}

struct ConeSensitivity {
    float wavelength, lCone, mCone, sCone;
};

// Function to interpolate sensitivity
float interpolate(const ConeSensitivity* data, int count, float lambda, int coneType) {
    for (int i = 0; i < count - 1; ++i) {
        if (lambda >= data[i].wavelength && lambda <= data[i + 1].wavelength) {
            // Compute interpolation ratio
            float ratio = (lambda - data[i].wavelength) / (data[i + 1].wavelength - data[i].wavelength);

            // Select the correct cone type: 0 = lCone, 1 = mCone, 2 = sCone
            float y1 = 0, y2 = 0;
            switch (coneType) {
                case 0: y1 = data[i].lCone; y2 = data[i + 1].lCone; break;
                case 1: y1 = data[i].mCone; y2 = data[i + 1].mCone; break;
                case 2: y1 = data[i].sCone; y2 = data[i + 1].sCone; break;
                default: return 0.0f;  // Invalid coneType
            }

            // Linear interpolation
            return y1 + ratio * (y2 - y1);
        }
    }
    return 0.0f;  // Out of range
}


// Stockman & Sharpe cone fundamentails, 10 deg, based on Stile & Burch 10 deg CMF
ConeSensitivity cones[] = {
    390,  4.07619E-04,  3.58227E-04,  6.14265E-03,
    395,  1.06921E-03,  9.64828E-04,  1.59515E-02,
    400,  2.54073E-03,  2.37208E-03,  3.96308E-02,
    405,  5.31546E-03,  5.12316E-03,  8.97612E-02,
    410,  9.98835E-03,  9.98841E-03,  1.78530E-01,
    415,  1.60130E-02,  1.72596E-02,  3.05941E-01,
    420,  2.33957E-02,  2.73163E-02,  4.62692E-01,
    425,  3.09104E-02,  3.96928E-02,  6.09570E-01,
    430,  3.97810E-02,  5.55384E-02,  7.56885E-01,
    435,  4.94172E-02,  7.50299E-02,  8.69984E-01,
    440,  5.94619E-02,  9.57612E-02,  9.66960E-01,
    445,  6.86538E-02,  1.16220E-01,  9.93336E-01,
    450,  7.95647E-02,  1.39493E-01,  9.91329E-01,
    455,  9.07704E-02,  1.62006E-01,  9.06735E-01,
    460,  1.06663E-01,  1.93202E-01,  8.23726E-01,
    465,  1.28336E-01,  2.32275E-01,  7.37043E-01,
    470,  1.51651E-01,  2.71441E-01,  6.10456E-01,
    475,  1.77116E-01,  3.10372E-01,  4.70894E-01,
    480,  2.07940E-01,  3.55066E-01,  3.50108E-01,
    485,  2.44046E-01,  4.05688E-01,  2.58497E-01,
    490,  2.82752E-01,  4.56137E-01,  1.85297E-01,
    495,  3.34786E-01,  5.22970E-01,  1.35351E-01,
    500,  3.91705E-01,  5.91003E-01,  9.67990E-02,
    505,  4.56252E-01,  6.66404E-01,  6.49614E-02,
    510,  5.26538E-01,  7.43612E-01,  4.12337E-02,
    515,  5.99867E-01,  8.16808E-01,  2.71300E-02,
    520,  6.75313E-01,  8.89214E-01,  1.76298E-02,
    525,  7.37108E-01,  9.34977E-01,  1.13252E-02,
    530,  7.88900E-01,  9.61962E-01,  7.17089E-03,
    535,  8.37403E-01,  9.81481E-01,  4.54287E-03,
    540,  8.90871E-01,  9.98931E-01,  2.83352E-03,
    545,  9.26660E-01,  9.91383E-01,  1.75573E-03,
    550,  9.44527E-01,  9.61876E-01,  1.08230E-03,
    555,  9.70703E-01,  9.35829E-01,  6.64512E-04,
    560,  9.85636E-01,  8.90949E-01,  4.08931E-04,
    565,  9.96979E-01,  8.40969E-01,  2.51918E-04,
    570,  9.99543E-01,  7.76526E-01,  1.55688E-04,
    575,  9.87057E-01,  7.00013E-01,  9.67045E-05,
    580,  9.57841E-01,  6.11728E-01,  6.04705E-05,
    585,  9.39781E-01,  5.31825E-01,  3.81202E-05,
    590,  9.06693E-01,  4.54142E-01,  2.42549E-05,
    595,  8.59605E-01,  3.76527E-01,  1.55924E-05,
    600,  8.03173E-01,  3.04378E-01,  1.01356E-05,
    605,  7.40680E-01,  2.39837E-01,  6.66657E-06,
    610,  6.68991E-01,  1.85104E-01,  4.43906E-06,
    615,  5.93248E-01,  1.40431E-01,  2.99354E-06,
    620,  5.17449E-01,  1.04573E-01, 0,
    625,  4.45125E-01,  7.65841E-02, 0,
    630,  3.69168E-01,  5.54990E-02, 0,
    635,  3.00316E-01,  3.97097E-02, 0,
    640,  2.42316E-01,  2.80314E-02, 0,
    645,  1.93730E-01,  1.94366E-02, 0,
    650,  1.49509E-01,  1.37660E-02, 0,
    655,  1.12638E-01,  9.54315E-03, 0,
    660,  8.38077E-02,  6.50455E-03, 0,
    665,  6.16384E-02,  4.42794E-03, 0,
    670,  4.48132E-02,  3.06050E-03, 0,
    675,  3.21660E-02,  2.11596E-03, 0,
    680,  2.27738E-02,  1.45798E-03, 0,
    685,  1.58939E-02,  9.98424E-04, 0,
    690,  1.09123E-02,  6.77653E-04, 0,
    695,  7.59453E-03,  4.67870E-04, 0,
    700,  5.28607E-03,  3.25278E-04, 0,
    705,  3.66675E-03,  2.25641E-04, 0,
    710,  2.51327E-03,  1.55286E-04, 0,
    715,  1.72108E-03,  1.07388E-04, 0,
    720,  1.18900E-03,  7.49453E-05, 0,
    725,  8.22396E-04,  5.24748E-05, 0,
    730,  5.72917E-04,  3.70443E-05, 0,
    735,  3.99670E-04,  2.62088E-05, 0,
    740,  2.78553E-04,  1.85965E-05, 0,
    745,  1.96528E-04,  1.33965E-05, 0,
    750,  1.38482E-04,  9.63397E-06, 0,
    755,  9.81226E-05,  6.96522E-06, 0,
    760,  6.98827E-05,  5.06711E-06, 0,
    765,  4.98430E-05,  3.68617E-06, 0,
    770,  3.57781E-05,  2.69504E-06, 0,
    775,  2.56411E-05,  1.96864E-06, 0,
    780,  1.85766E-05,  1.45518E-06, 0,
    785,  1.34792E-05,  1.07784E-06, 0,
    790,  9.80671E-06,  8.00606E-07, 0,
    795,  7.14808E-06,  5.96195E-07, 0,
    800,  5.24229E-06,  4.48030E-07, 0,
    805,  3.86280E-06,  3.38387E-07, 0,
    810,  2.84049E-06,  2.54942E-07, 0,
    815,  2.10091E-06,  1.93105E-07, 0,
    820,  1.56506E-06,  1.47054E-07, 0,
    825,  1.16700E-06,  1.11751E-07, 0,
    830,  8.73008E-07,  8.48902E-08, 0,
};


CMFMatch match10[] = {
    390,2.952420E-03,4.076779E-04,1.318752E-02,
    395,7.641137E-03,1.078166E-03,3.424588E-02,
    400,1.879338E-02,2.589775E-03,8.508254E-02,
    405,4.204986E-02,5.474207E-03,1.927065E-01,
    410,8.277331E-02,1.041303E-02,3.832822E-01,
    415,1.395127E-01,1.712968E-02,6.568187E-01,
    420,2.077647E-01,2.576133E-02,9.933444E-01,
    425,2.688989E-01,3.529554E-02,1.308674E+00,
    430,3.281798E-01,4.698226E-02,1.624940E+00,
    435,3.693084E-01,6.047429E-02,1.867751E+00,
    440,4.026189E-01,7.468288E-02,2.075946E+00,
    445,4.042529E-01,8.820537E-02,2.132574E+00,
    450,3.932139E-01,1.039030E-01,2.128264E+00,
    455,3.482214E-01,1.195389E-01,1.946651E+00,
    460,3.013112E-01,1.414586E-01,1.768440E+00,
    465,2.534221E-01,1.701373E-01,1.582342E+00,
    470,1.914176E-01,1.999859E-01,1.310576E+00,
    475,1.283167E-01,2.312426E-01,1.010952E+00,
    480,7.593120E-02,2.682271E-01,7.516389E-01,
    485,3.836770E-02,3.109438E-01,5.549619E-01,
    490,1.400745E-02,3.554018E-01,3.978114E-01,
    495,3.446810E-03,4.148227E-01,2.905816E-01,
    500,5.652072E-03,4.780482E-01,2.078158E-01,
    505,1.561956E-02,5.491344E-01,1.394643E-01,
    510,3.778185E-02,6.248296E-01,8.852389E-02,
    515,7.538941E-02,7.012292E-01,5.824484E-02,
    520,1.201511E-01,7.788199E-01,3.784916E-02,
    525,1.756832E-01,8.376358E-01,2.431375E-02,
    530,2.380254E-01,8.829552E-01,1.539505E-02,
    535,3.046991E-01,9.233858E-01,9.753000E-03,
    540,3.841856E-01,9.665325E-01,6.083223E-03,
    545,4.633109E-01,9.886887E-01,3.769336E-03,
    550,5.374170E-01,9.907500E-01,2.323578E-03,
    555,6.230892E-01,9.997775E-01,1.426627E-03,
    560,7.123849E-01,9.944304E-01,8.779264E-04,
    565,8.016277E-01,9.848127E-01,5.408385E-04,
    570,8.933408E-01,9.640545E-01,3.342429E-04,
    575,9.721304E-01,9.286495E-01,2.076129E-04,
    580,1.034327E+00,8.775360E-01,1.298230E-04,
    585,1.106886E+00,8.370838E-01,8.183954E-05,
    590,1.147304E+00,7.869950E-01,5.207245E-05,
    595,1.160477E+00,7.272309E-01,3.347499E-05,
    600,1.148163E+00,6.629035E-01,2.175998E-05,
    605,1.113846E+00,5.970375E-01,1.431231E-05,
    610,1.048485E+00,5.282296E-01,9.530130E-06,
    615,9.617111E-01,4.601308E-01,6.426776E-06,
    620,8.629581E-01,3.950755E-01,0.000000E+00,
    625,7.603498E-01,3.351794E-01,0.000000E+00,
    630,6.413984E-01,2.751807E-01,0.000000E+00,
    635,5.290979E-01,2.219564E-01,0.000000E+00,
    640,4.323126E-01,1.776882E-01,0.000000E+00,
    645,3.496358E-01,1.410203E-01,0.000000E+00,
    650,2.714900E-01,1.083996E-01,0.000000E+00,
    655,2.056507E-01,8.137687E-02,0.000000E+00,
    660,1.538163E-01,6.033976E-02,0.000000E+00,
    665,1.136072E-01,4.425383E-02,0.000000E+00,
    670,8.281010E-02,3.211852E-02,0.000000E+00,
    675,5.954815E-02,2.302574E-02,0.000000E+00,
    680,4.221473E-02,1.628841E-02,0.000000E+00,
    685,2.948752E-02,1.136106E-02,0.000000E+00,
    690,2.025590E-02,7.797457E-03,0.000000E+00,
    695,1.410230E-02,5.425391E-03,0.000000E+00,
    700,9.816228E-03,3.776140E-03,0.000000E+00,
    705,6.809147E-03,2.619372E-03,0.000000E+00,
    710,4.666298E-03,1.795595E-03,0.000000E+00,
    715,3.194041E-03,1.229980E-03,0.000000E+00,
    720,2.205568E-03,8.499903E-04,0.000000E+00,
    725,1.524672E-03,5.881375E-04,0.000000E+00,
    730,1.061495E-03,4.098928E-04,0.000000E+00,
    735,7.400120E-04,2.860718E-04,0.000000E+00,
    740,5.153113E-04,1.994949E-04,0.000000E+00,
    745,3.631969E-04,1.408466E-04,0.000000E+00,
    750,2.556624E-04,9.931439E-05,0.000000E+00,
    755,1.809649E-04,7.041878E-05,0.000000E+00,
    760,1.287394E-04,5.018934E-05,0.000000E+00,
    765,9.172477E-05,3.582218E-05,0.000000E+00,
    770,6.577532E-05,2.573083E-05,0.000000E+00,
    775,4.708916E-05,1.845353E-05,0.000000E+00,
    780,3.407653E-05,1.337946E-05,0.000000E+00,
    785,2.469630E-05,9.715798E-06,0.000000E+00,
    790,1.794555E-05,7.074424E-06,0.000000E+00,
    795,1.306345E-05,5.160948E-06,0.000000E+00,
    800,9.565993E-06,3.788729E-06,0.000000E+00,
    805,7.037621E-06,2.794625E-06,0.000000E+00,
    810,5.166853E-06,2.057152E-06,0.000000E+00,
    815,3.815429E-06,1.523114E-06,0.000000E+00,
    820,2.837980E-06,1.135758E-06,0.000000E+00,
    825,2.113325E-06,8.476168E-07,0.000000E+00,
    830,1.579199E-06,6.345380E-07,0.000000E+00,
};

// The plotting function for ImGui/ImPlot
void ShowSensitivityPlot() {
    ImGui::Begin("Sensitivity Plot");

    if (ImPlot::BeginPlot("Photoreceptor Sensitivity",
                          ImVec2(-1, -1), ImPlotFlags_NoLegend)) {
        ImPlot::SetupLegend(ImPlotLocation_NorthEast,
                            ImPlotLegendFlags_None);//ImPlotLegendFlags_Outside);
        ImPlot::SetupAxes("Wavelength (nm)", "Sensitivity");

        int count = sizeof(cones) / sizeof(ConeSensitivity);
        static double lambda_vals[100];   // X-axis: wavelengths
        static double cone_L_vals[100];   // Y-axis: S_cone_L
        static double cone_M_vals[100];   // Y-axis: S_cone_M
        static double cone_S_vals[100];   // Y-axis: S_cone_S
        //static double rod_vals[100];      // Y-axis: S_rod

        // Fill data
        for (int i = 0; i < 100; ++i) {
            double lambda = 380 + i * 3.2;  // Sampling from 380nm to 700nm
            lambda_vals[i] = lambda;
            cone_L_vals[i] = interpolate(cones, count, lambda, 0);
            cone_M_vals[i] = interpolate(cones, count, lambda, 1);
            cone_S_vals[i] = interpolate(cones, count, lambda, 2);
            //rod_vals[i] = interpolate(lambda, wavelengths, s_rod, count);
        }

        // Plot each sensitivity function
        ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImPlot::PlotLine("S_cone_L (red)", lambda_vals, cone_L_vals, 100);
        ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        ImPlot::PlotLine("S_cone_M (green)", lambda_vals, cone_M_vals, 100);
        ImPlot::SetNextLineStyle(ImVec4(0.1f, 0.3f, 1.0f, 1.0f));
        ImPlot::PlotLine("S_cone_S (blue)", lambda_vals, cone_S_vals, 100);
        //ImPlot::PlotLine("S_rod", lambda_vals, rod_vals, 100);

        ImPlot::EndPlot();
    }
    ImGui::End();
}


// Function to interpolate CMF data based on wavelength
CMFMatch interpolateCMF(const CMFMatch* data, int count, double wavelength) {
    // Clamp the wavelength to the bounds of the data
    if (wavelength <= data[0].wavelength) return data[0];
    if (wavelength >= data[count - 1].wavelength) return data[count - 1];

    // Find the two closest points for interpolation
    for (int i = 0; i < count - 1; ++i) {
        if (wavelength >= data[i].wavelength && wavelength <= data[i + 1].wavelength) {
            double t = (wavelength - data[i].wavelength) / (data[i + 1].wavelength - data[i].wavelength);
            CMFMatch result;
            result.wavelength = wavelength;
            result.x = data[i].x + t * (data[i + 1].x - data[i].x);
            result.y = data[i].y + t * (data[i + 1].y - data[i].y);
            result.z = data[i].z + t * (data[i + 1].z - data[i].z);
            return result;
        }
    }
    return data[0]; // Fallback, should never happen if inputs are valid
}

// Function to plot the 10-degree CMF data
void ShowColorMatchPlot10Degree(const CMFMatch* data, int count) {
    ImGui::Begin("10-Degree CMF Plot");
    if (ImPlot::BeginPlot("CIE 1964 10-degree CMF",
                          ImVec2(-1, -1), ImPlotFlags_NoLegend)) {
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_None);
        ImPlot::SetupAxes("Wavelength (nm)", "Matching Function");
        //ImPlotLegendFlags_Outside);

        static double lambda_vals[100];   // X-axis: wavelengths
        static double x_vals[100];        // Y-axis: x-bar
        static double y_vals[100];        // Y-axis: y-bar
        static double z_vals[100];        // Y-axis: z-bar

        // Fill data arrays using interpolation
        for (int i = 0; i < 100; ++i) {
            double lambda = 390 + i * 3.1;  // Sampling from 390nm to 700nm
            lambda_vals[i] = lambda;
            CMFMatch interpolated = interpolateCMF(data, count, lambda);
            x_vals[i] = interpolated.x;  // x-bar
            y_vals[i] = interpolated.y;  // y-bar
            z_vals[i] = interpolated.z;  // z-bar
        }

        // Force red color for x-bar (X)
        ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));  // Red color
        ImPlot::PlotLine("x-bar (red)", lambda_vals, x_vals, 100);

        // Force green color for y-bar (Y)
        ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f));  // Green color
        ImPlot::PlotLine("y-bar (green)", lambda_vals, y_vals, 100);

        // Force blue color for z-bar (Z)
        ImPlot::SetNextLineStyle(ImVec4(0.1f, 0.3f, 1.0f, 1.0f));
        ImPlot::PlotLine("z-bar (blue)", lambda_vals, z_vals, 100);

        ImPlot::EndPlot();
    }
    ImGui::End();
}

void PlotTick(double wavelength, const CMFMatch* cmfData, int dataSize, double tickLength) {
    // Get the CMF normalized position for the given wavelength
    CMFMatch cmf1 = interpolateCMF(cmfData, dataSize, wavelength);
    CMFMatch cmf2 = interpolateCMF(cmfData, dataSize, wavelength + 2.0);

    // Normalize cmf1 and cmf2 to get chromaticity coordinates
    double sum1 = cmf1.x + cmf1.y + cmf1.z;
    double normX1 = cmf1.x / sum1;
    double normY1 = cmf1.y / sum1;

    double sum2 = cmf2.x + cmf2.y + cmf2.z;
    double normX2 = cmf2.x / sum2;
    double normY2 = cmf2.y / sum2;

    // Calculate the direction vector
    double dx = normX2 - normX1;
    double dy = normY2 - normY1;

    // Compute the perpendicular vector
    double perpX = -dy;
    double perpY = dx;

    // Normalize the perpendicular vector and scale by tickLength
    double length = sqrt(perpX * perpX + perpY * perpY);
    perpX = (perpX / length) * tickLength;
    perpY = (perpY / length) * tickLength;

    // Tick position (start point for the tick mark)
    double tickX = normX1;
    double tickY = normY1;

    // Create arrays for the tick line
    double x_vals[2];
    double y_vals[2];

    // Fill in the start and end points for the tick
    x_vals[0] = tickX;
    y_vals[0] = tickY;
    x_vals[1] = tickX + perpX;
    y_vals[1] = tickY + perpY;

    // Draw the tick mark as a single line
    ImPlot::PlotLine("Tick", x_vals, y_vals, 2);

    // Compute label position
    double labelX = tickX + 1.1 * perpX;
    //double labelX = tickX + (perpX > 0 ? 1.1 * perpX : -1.1 * tickLength);
    double labelY = tickY + 1.1 * perpY;
    //double labelY = tickY + (perpX > 0 ? 1.1 * perpY : -1.1 * tickLength);

    // Use PlotText to place the wavelength label
    ImPlot::PlotText(std::to_string(static_cast<int>(wavelength)).c_str(), labelX, labelY);
}

// Function to plot the spectral locus
void ColorActivity::data::PlotSpectralLocus(const CMFMatch* cmfData, int dataSize) {
    ImGui::Begin("CIE1964 10 degree Spectral Locus Plot");
    ImVec2 sz = ImGui::GetWindowSize();

    const int numSamples = 100; // Number of wavelength samples
    XY vals[numSamples];
    float x_vals[numSamples];
    float y_vals[numSamples];
    float wavelengths[numSamples];

    float min_wavelength = 380.0;
    float max_wavelength = 700.0;

    const NcColorSpace* lin_cs = NcGetNamedColorSpace(working_cs.c_str());
    // populate vals[] with the chromaticity coordinates
    NcColorSpaceDescriptor lin_cs_desc;
    NcGetColorSpaceDescriptor(lin_cs, &lin_cs_desc);
    vals[0] = {lin_cs_desc.redPrimary.x, lin_cs_desc.redPrimary.y};
    vals[1] = {lin_cs_desc.greenPrimary.x, lin_cs_desc.greenPrimary.y};
    vals[2] = {lin_cs_desc.bluePrimary.x, lin_cs_desc.bluePrimary.y};

    // Sweep visible spectrum
    for (int i = 0; i < numSamples; ++i) {
        float wavelength = min_wavelength + (max_wavelength - min_wavelength) * i / (numSamples - 1);
        wavelengths[i] = wavelength;

        // Get interpolated CMF values
        CMFMatch cmf = interpolateCMF(cmfData, dataSize, wavelength);

        // Compute X, Y, Z (assuming Luminance = 1 for simplicity)
        float X = cmf.x;
        float Y = cmf.y;
        float Z = cmf.z;

        // Normalize to get chromaticity coordinates (x, y)
        float sum = X + Y + Z;
        x_vals[i] = X / sum;
        y_vals[i] = Y / sum;
        //vals[i] = {x_vals[i], y_vals[i]};
    }

    const int numPlanckSamples = 32;
    float planck_x[numPlanckSamples];
    float planck_y[numPlanckSamples];
    // NcKelvinToYxy is valid between 1000 and 15000k. Generate 32 samples.
    for (int i = 0; i < numPlanckSamples; ++i) {
        float temperature = 1000 + (15000 - 1000) * i / (numPlanckSamples - 1);
        NcYxy Yxy = NcKelvinToYxy(temperature, 1.f);
        planck_x[i] = Yxy.x;
        planck_y[i] = Yxy.y;
    }

    if (ImPlot::BeginPlot("Spectral Locus", ImVec2(-1, -1),
                          ImPlotFlags_NoLegend | ImPlotFlags_Equal)) {
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_None);
        ImPlot::SetupAxes("x", "y");

        if (show_gamuts) {
            // Define the chromaticity boundaries and sample grid dimensions
            float x_start = 0.0f, y_start = 0.0f;
            float x_end = 1.0f, y_end = 1.0f;
            int grid_x = 60, grid_y = 60; // Number of grid cells

            // Push the plot clipping region so we don't draw outside the plot
            ImPlot::PushPlotClipRect();

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Calculate grid cell size
            float cell_width = (x_end - x_start) / grid_x;
            float cell_height = (y_end - y_start) / grid_y;

            // Loop through grid to draw colored squares
            for (int i = 0; i < grid_x; i++) {
                for (int j = 0; j < grid_y; j++) {
                    // Calculate the top-left and bottom-right corners in plot coordinates
                    ImVec2 top_left = ImPlot::PlotToPixels(x_start + i * cell_width, y_start + j * cell_height);
                    ImVec2 bottom_right = ImPlot::PlotToPixels(x_start + (i + 1) * cell_width, y_start + (j + 1) * cell_height);

                    float x = i / float(grid_x);
                    float y = j / float(grid_y);

                    // check if (x, y) is inside the horseshoe
                    XY testP = {x + cell_width * 0.5f, y + cell_height * 0.5f};
                    if (windingNumber(testP, vals, 3 /*numSamples*/)!= 0) {

                        NcRGB rgb = NcYxyToRGB(lin_cs, {1, x, y});

                        // Generate color based on position or other criteria
                        ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(rgb.r, rgb.g, rgb.b, 1.0f));

                        // Draw filled rectangle
                        draw_list->AddRectFilled(top_left, bottom_right, color);
                    }
                }
            }

            // Pop the clipping rect to restore regular plotting behavior
            ImPlot::PopPlotClipRect();
        }

        ImPlot::PlotLine("Locus", x_vals, y_vals, numSamples);

        int step = 10;
        if (sz.y < 300)
            step = 25;
        // Draw ticks for significant wavelengths
        for (int i = 400; i < 700; i += step) {
            PlotTick(double(i), cmfData, dataSize, 0.05); // Tick length can be adjusted
        }

        ImPlot::PlotText("o", magook.x, magook.y);

        if (planckian_visible) {
            // Draw Planckian locus
            ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
            ImPlot::PlotLine("Planckian Locus", planck_x, planck_y, numPlanckSamples);
        }

        ImPlot::EndPlot();
    }
    ImGui::End();
}

struct Knot {
    double wavelength; // nm
    double value;     // response value
};

// Global variable to hold knots
std::vector<Knot> knots = {
    {380, 0.0}, {400, 0.0}, {450, 0.0}, {500, 0.0}, {550, 0.0}, {600, 0.0}, {650, 0.0}, {700, 0.0}
};


double InterpolateValue(const std::vector<double>& wavelengths, const std::vector<double>& values, double targetWavelength) {
    // Ensure the vectors are sorted and not empty
    if (wavelengths.empty() || values.empty() || wavelengths.size() != values.size()) {
        throw std::invalid_argument("Wavelengths and values must be non-empty and of the same size.");
    }

    if (targetWavelength <= wavelengths.front())
        return values.front();
    if (targetWavelength >= wavelengths.back())
        return values.back();

    // Find the two surrounding wavelengths
    for (size_t i = 0; i < wavelengths.size() - 1; ++i) {
        if (targetWavelength >= wavelengths[i] && targetWavelength <= wavelengths[i + 1]) {
            // Perform linear interpolation
            double t = (targetWavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
            return values[i] + t * (values[i + 1] - values[i]);
        }
    }

    // If we reach here, something went wrong
    throw std::runtime_error("Interpolation failed. Check the input data.");
}

// Function to plot the spectral response curve
void PlotSpectralResponseCurve(const CMFMatch* cmfData, int dataSize) {
    ImGui::Begin("Spectral Response Curve");
    // Create arrays for plotting
    std::vector<double> wavelengths;
    std::vector<double> values;
    if (ImPlot::BeginPlot("Response Curve", ImVec2(-1, 300),
                          ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes("Wavelength (nm)", "Response");

        for (const auto& knot : knots) {
            wavelengths.push_back(knot.wavelength);
            values.push_back(knot.value);
        }

        // Plot the line graph
        ImPlot::PlotLine("Response", wavelengths.data(), values.data(),
                         (int)wavelengths.size());

#if 0
        // Handle knot interactions
        if (ImGui::IsMouseClicked(0) && ImPlot::IsPlotHovered()) {
            // Add new knot on click
            ImVec2 mousePos = ImGui::GetMousePos();
            ImPlotPoint plotPos = ImPlot::GetPlotMousePos();
            if (plotPos.x >= 0) { // Ensure we're within bounds
                knots.push_back({plotPos.x, plotPos.y});
                std::sort(knots.begin(), knots.end(), [](const Knot& a, const Knot& b) {
                    return a.wavelength < b.wavelength;
                });
            }
        }
#endif

        // Allow dragging knots using ImPlot::DragPoint
        for (size_t i = 0; i < knots.size(); ++i) {
            ImPlotPoint knotPos = {knots[i].wavelength, knots[i].value};

            // Use DragPoint for moving knots
            ImPlot::DragPoint((int) i, &knotPos.x, &knotPos.y,
                              ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 5.0f, ImPlotDragToolFlags_None);

            // Update the knot's position if it was dragged
            //knots[i].wavelength = knotPos.x;
            knots[i].value = knotPos.y > 0 ? knotPos.y : 0.0;
        }

        // Right-click menu for removing knots
        if (ImGui::IsMouseReleased(1) && ImPlot::IsPlotHovered()) {
            for (size_t i = 0; i < knots.size(); ++i) {
                ImPlotPoint knotPos = {knots[i].wavelength, knots[i].value};
                if (std::abs(knotPos.x - ImPlot::GetPlotMousePos().x) < 5.0 &&
                    std::abs(knotPos.y - ImPlot::GetPlotMousePos().y) < 5.0) {
                    ImGui::OpenPopup("KnotContextMenu");
                    if (ImGui::BeginPopup("KnotContextMenu")) {
                        if (ImGui::MenuItem("Remove Knot")) {
                            knots.erase(knots.begin() + i);
                        }
                        ImGui::EndPopup();
                    }
                }
            }
        }

        ImPlot::EndPlot();
    }

    if (wavelengths.size()) {
        CMFMatch r {0,1e-6,0}; // start with a tiny value to avoid nan
        for (int i = 380; i <=700; i += 10) {
            CMFMatch cmf1 = interpolateCMF(cmfData, dataSize, double(i));
            double value = InterpolateValue(wavelengths, values, double(i));
            if (value > 0) {
                r.x += cmf1.x * value;
                r.y += cmf1.y * value;
                r.z += cmf1.z * value;
            }
        }
        double normX = r.x / (r.x + r.y + r.z);
        double normY = r.y / (r.x + r.y + r.z);

        magook.x = float(normX);
        magook.y = float(normY);

        ImGui::Text("CIEXY Chromaticity: ");
        ImGui::Text("X: %.4f", float(normX));
        ImGui::Text("Y: %.4f", float(normY));
    }

    ImGui::End();
}

void ColorActivity::RunUI(const LabViewInteraction&)
{
    ImGui::Begin("Color Activity Settings##mM");
    ImGui::Checkbox("Render perceptual color", &_self->horseshoe_visible);
    if (_self->horseshoe_visible) {
        ImGui::Checkbox("Show ISO17321 chart", &_self->macbeth_visible);
        ImGui::Checkbox("Fill the chart", &_self->fill_chart);
        ImGui::Checkbox("Show Planckian locus", &_self->planckian_visible);
        ImGui::Checkbox("Show gamuts", &_self->show_gamuts);
    }
    const char* result = _self->working_cs.c_str();
    ImGui::Separator();
    if (ColorSpacePicker("rendering color space", &_self->cs_index, &result)) {
        _self->working_cs.assign(result);
    }
    ImGui::End();

    ShowSensitivityPlot();
    ShowColorMatchPlot10Degree(match10, sizeof(match10)/sizeof(CMFMatch));
    _self->PlotSpectralLocus(match10, sizeof(match10)/sizeof(CMFMatch));
    PlotSpectralResponseCurve(match10, sizeof(match10)/sizeof(CMFMatch));
}


} //lab

