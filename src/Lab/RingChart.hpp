#ifndef RINGCHART_HPP
#define RINGCHART_HPP


#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>
struct Vec2 {
    float x, y;
};

inline float DistanceSquared(const Vec2& a, const Vec2& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

inline float AngleFromCenter(const Vec2& center, const Vec2& p) {
    return std::atan2(p.y - center.y, p.x - center.x);
}

enum class ChartMode {
    Ring,
    Flame
};

struct RingSlice {
    // Ring layout
    float a0, a1;          // Start and end angles in radians
    float r0, r1;          // Inner and outer radii

    // Flame layout
    float x0 = 0, x1 = 0;
    float y0 = 0, y1 = 0;

    Vec2 center;           // Chart center
    unsigned int color;    // RGBA as 0xRRGGBBAA
    void* userData = nullptr;  // Optional: link to domain object (e.g., UsdPrim)

    // Morph interpolation (0 = ring, 1 = flame)
    float morphT = 0.0f;

    bool contains(const Vec2& p, ChartMode mode) const {
        return (mode == ChartMode::Ring) ? containsRing(p) : containsFlame(p);
    }

    void GetInterpolatedQuad(Vec2 out[4]) const {
        float t = morphT;
        float cx = center.x;
        float cy = center.y;

        float a0_ = a0, a1_ = a1;
        if (a0_ < 0) a0_ += 2 * 3.14159265f;
        if (a1_ < 0) a1_ += 2 * 3.14159265f;

        // Interpolated coordinates
        float fx0 = (1 - t) * (cx + r0 * std::cos(a0_)) + t * x0;
        float fy0 = (1 - t) * (cy + r0 * std::sin(a0_)) + t * y0;

        float fx1 = (1 - t) * (cx + r1 * std::cos(a0_)) + t * x0;
        float fy1 = (1 - t) * (cy + r1 * std::sin(a0_)) + t * y1;

        float fx2 = (1 - t) * (cx + r1 * std::cos(a1_)) + t * x1;
        float fy2 = (1 - t) * (cy + r1 * std::sin(a1_)) + t * y1;

        float fx3 = (1 - t) * (cx + r0 * std::cos(a1_)) + t * x1;
        float fy3 = (1 - t) * (cy + r0 * std::sin(a1_)) + t * y0;

        out[0] = { fx0, fy0 };
        out[1] = { fx1, fy1 };
        out[2] = { fx2, fy2 };
        out[3] = { fx3, fy3 };
    }

private:
    bool containsRing(const Vec2& p) const {
        float dx = p.x - center.x;
        float dy = p.y - center.y;
        float distSq = dx * dx + dy * dy;
        float r0Sq = r0 * r0;
        float r1Sq = r1 * r1;
        if (distSq < r0Sq || distSq > r1Sq)
            return false;
        float angle = std::atan2(dy, dx);
        if (angle < 0) angle += 2 * 3.14159265f;
        float a0Norm = std::fmod(a0, 2 * 3.14159265f);
        if (a0Norm < 0) a0Norm += 2 * 3.14159265f;
        float a1Norm = std::fmod(a1, 2 * 3.14159265f);
        if (a1Norm < 0) a1Norm += 2 * 3.14159265f;
        if (a0Norm < a1Norm)
            return angle >= a0Norm && angle <= a1Norm;
        else
            return angle >= a0Norm || angle <= a1Norm;
    }

    bool containsFlame(const Vec2& p) const {
        return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1;
    }
};

class Ring {
public:
    std::vector<RingSlice> elements;
};


class RingChart {
public:
    std::vector<Ring> rings;
    
    /// Build both ring and flame chart layouts given hierarchical ratios.
    /// Each level will expand outward from r0 by ringThickness or stack vertically.
    void BuildFromData(
            const std::vector<std::vector<float>>& hierarchy,
            float r0, float ringThickness,
            float a0 = 0.0f, float a1 = 6.283185f,
            Vec2 chartCenter = {0.0f, 0.0f})
    {
        rings.clear();
        float currentR0 = r0;
        float flameY = 0.0f;
        for (const auto& level : hierarchy) {
            rings.push_back({});
            auto& ring = rings.back();
            float total = 0.0f;
            for (float v : level)
                total += v;
            if (total <= 0.0f) {
                currentR0 += ringThickness;
                flameY += ringThickness;
                continue;
            }
            float cursorAngle = a0;
            float cursorX = 0.0f;
            for (float v : level) {
                float angleSpan = (v / total) * (a1 - a0);
                float width = v / total; // Normalized width for flame

                RingSlice slice;
                slice.center = chartCenter;

                // Ring layout
                slice.r0 = currentR0;
                slice.r1 = currentR0 + ringThickness;
                slice.a0 = cursorAngle;
                slice.a1 = cursorAngle + angleSpan;

                // Flame layout
                slice.x0 = cursorX;
                slice.x1 = cursorX + width;
                slice.y0 = flameY;
                slice.y1 = flameY + ringThickness;

                slice.color = 0xffffffff; // placeholder white
                ring.elements.push_back(slice);

                cursorAngle += angleSpan;
                cursorX += width;
            }
            currentR0 += ringThickness;
            flameY += ringThickness;
        }
    }

    const std::vector<Ring>& GetRings() const { return rings; }

    void SetMorph(float t) {
        for (auto& ring : rings) {
            for (auto& e : ring.elements) {
                e.morphT = std::clamp(t, 0.0f, 1.0f);
            }
        }
    }

    /// Determine which slice (if any) contains the point p, using current mode
    RingSlice* HitTest(const Vec2& p, ChartMode mode) {
        for (auto& ring : rings) {
            for (auto& e : ring.elements) {
                if (e.contains(p, mode))
                    return &e;
            }
        }
        return nullptr;
    }
};

void DrawRingChartInteractive(RingChart& chart, ChartMode mode, float morphT, float scale = 1.0f);

#endif // RINGCHART_HPP
