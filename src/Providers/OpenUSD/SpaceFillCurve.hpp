//////////////////////////////////////////////////////////////////////
// SpaceFillCurve.h - Space-filling curve conversions, esp. Morton and Hilbert
//
// Copyright David K. McAllister, 2012-2022.
// CC0 1.0 Universal
// source https://github.com/davemc0/DMcTools

// Count of random point pairs with coord distance <= 9 that had a code difference < 100 with 10 bits per dimension
// Computed with DMcToolsTest -SpaceFillCurveTest testCoordToCodeDistAll
// CURVE_MORTON  750,100
// CURVE_HILBERT 824,710
// CURVE_RASTER   58,906
// CURVE_BOUSTRO  66,123
// CURVE_TILED2  358,118

#pragma once
#include <vector>

//#include "Math/AABB.h"
#define DMC_DECL
#define ASSERTVEC(a)


template <class T, int L, class S> class tVector {
public:
    static const int Chan = L; // The number of channels/components/dimensions in the vector

    DMC_DECL tVector() {}

    DMC_DECL const T* data() const { return ((S*)this)->data(); }
    DMC_DECL T* data() { return ((S*)this)->data(); }
    DMC_DECL const T& get(int idx) const
    {
        ASSERTVEC(idx >= 0 && idx < L);
        return data()[idx];
    }
    DMC_DECL T& get(int idx)
    {
        ASSERTVEC(idx >= 0 && idx < L);
        return data()[idx];
    }
    DMC_DECL T set(int idx, const T& a)
    {
        T& slot = get(idx);
        T old = slot;
        slot = a;
        return old;
    }

    DMC_DECL void set(const T& a)
    {
        T* tp = data();
        for (int i = 0; i < L; i++) tp[i] = a;
    }
    DMC_DECL void set(const T* ptr)
    {
        ASSERTVEC(ptr);
        T* tp = data();
        for (int i = 0; i < L; i++) tp[i] = ptr[i];
    }
    DMC_DECL void setZero() { set((T)0); }
    template <class V> DMC_DECL void set(const tVector<T, L, V>& v) { set(v.data()); }

    // Reduction const operations across components
    DMC_DECL T min() const
    {
        const T* tp = data();
        T r = tp[0];
        for (int i = 1; i < L; i++) r = ::min(r, tp[i]); // Use min defined with above macro
        return r;
    }
    DMC_DECL T max() const
    {
        const T* tp = data();
        T r = tp[0];
        for (int i = 1; i < L; i++) r = ::max(r, tp[i]); // Use max defined with above macro
        return r;
    }
    DMC_DECL T sum() const
    {
        const T* tp = data();
        T r = tp[0];
        for (int i = 1; i < L; i++) r += tp[i];
        return r;
    }
    DMC_DECL int minDim() const // Return index of min dimension
    {
        const T* tp = data();
        T r = tp[0];
        int dim = 0;
        for (int i = 1; i < L; i++)
            if (tp[i] < r) {
                r = tp[i];
                dim = i;
            }
        return dim;
    }
    DMC_DECL int maxDim() const // Return index of max dimension
    {
        const T* tp = data();
        T r = tp[0];
        int dim = 0;
        for (int i = 1; i < L; i++)
            if (tp[i] > r) {
                r = tp[i];
                dim = i;
            }
        return dim;
    }
    template <class V> DMC_DECL bool operator==(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        for (int i = 0; i < L; i++)
            if (tp[i] != vp[i]) return false;
        return true;
    }
    template <class V> DMC_DECL bool operator!=(const tVector<T, L, V>& v) const { return (!operator==(v)); }
    DMC_DECL bool any_inf() const
    {
        const T* tp = data();
        for (int i = 0; i < L; i++)
            if (std::isinf(tp[i])) return true;
        return false;
    }
    DMC_DECL bool all_inf() const
    {
        const T* tp = data();
        for (int i = 0; i < L; i++)
            if (!std::isinf(tp[i])) return false;
        return true;
    }
    DMC_DECL bool any_nan() const
    {
        const T* tp = data();
        for (int i = 0; i < L; i++)
            if (std::isnan(tp[i])) return true;
        return false;
    }
    DMC_DECL bool all_nan() const
    {
        const T* tp = data();
        for (int i = 0; i < L; i++)
            if (!std::isnan(tp[i])) return false;
        return true;
    }
    DMC_DECL bool any_zero() const
    {
        const T* tp = data();
        for (int i = 0; i < L; i++)
            if (tp[i] == (T)0) return true;
        return false;
    }
    DMC_DECL bool all_zero() const
    {
        const T* tp = data();
        for (int i = 0; i < L; i++)
            if (tp[i] != (T)0) return false;
        return true;
    }

    // Distance-ralated const and non-const operations
    DMC_DECL T lenSqr() const
    {
        const T* tp = data();
        T r = (T)0;
        for (int i = 0; i < L; i++) r += sqr(tp[i]);
        return r;
    }
    DMC_DECL T length() const { return sqrt(lenSqr()); }
    DMC_DECL S normalized(T len = (T)1) const { return operator*(len* rcp(length())); }
    DMC_DECL void normalize(T len = (T)1) { set(normalized(len)); }

    // Vector-scalar non-const operations
    DMC_DECL const T& operator[](int idx) const { return get(idx); }
    DMC_DECL T& operator[](int idx) { return get(idx); }

    DMC_DECL S& operator=(const T& a)
    {
        set(a);
        return *(S*)this;
    }
    DMC_DECL S& operator+=(const T& a)
    {
        set(operator+(a));
        return *(S*)this;
    }
    DMC_DECL S& operator-=(const T& a)
    {
        set(operator-(a));
        return *(S*)this;
    }
    DMC_DECL S& operator*=(const T& a)
    {
        set(operator*(a));
        return *(S*)this;
    }
    DMC_DECL S& operator/=(const T& a)
    {
        set(operator/(a));
        return *(S*)this;
    }
    DMC_DECL S& operator%=(const T& a)
    {
        set(operator%(a));
        return *(S*)this;
    }
    DMC_DECL S& operator&=(const T& a)
    {
        set(operator&(a));
        return *(S*)this;
    }
    DMC_DECL S& operator|=(const T& a)
    {
        set(operator|(a));
        return *(S*)this;
    }
    DMC_DECL S& operator^=(const T& a)
    {
        set(operator^(a));
        return *(S*)this;
    }
    DMC_DECL S& operator<<=(const T& a)
    {
        set(operator<<(a));
        return *(S*)this;
    }
    DMC_DECL S& operator>>=(const T& a)
    {
        set(operator>>(a));
        return *(S*)this;
    }

    // Unary and vector-scalar const operations
    DMC_DECL S operator+() const { return *this; }
    DMC_DECL S operator-() const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = -tp[i];
        return r;
    }
    DMC_DECL S operator~() const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = ~tp[i];
        return r;
    }
    DMC_DECL S operator+(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] + a;
        return r;
    }
    DMC_DECL S operator-(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] - a;
        return r;
    }
    DMC_DECL S operator*(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] * a;
        return r;
    }
    DMC_DECL S operator/(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] / a;
        return r;
    }
    DMC_DECL S operator%(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] % a;
        return r;
    }
    DMC_DECL S operator&(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] & a;
        return r;
    }
    DMC_DECL S operator|(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] | a;
        return r;
    }
    DMC_DECL S operator^(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] ^ a;
        return r;
    }
    DMC_DECL S operator<<(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] << a;
        return r;
    }
    DMC_DECL S operator>>(const T& a) const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] >> a;
        return r;
    }

    // Vector-vector component-wise non-const operations
    template <class V> DMC_DECL S& operator=(const tVector<T, L, V>& v)
    {
        set(v);
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator+=(const tVector<T, L, V>& v)
    {
        set(operator+(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator-=(const tVector<T, L, V>& v)
    {
        set(operator-(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator*=(const tVector<T, L, V>& v)
    {
        set(operator*(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator/=(const tVector<T, L, V>& v)
    {
        set(operator/(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator%=(const tVector<T, L, V>& v)
    {
        set(operator%(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator&=(const tVector<T, L, V>& v)
    {
        set(operator&(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator|=(const tVector<T, L, V>& v)
    {
        set(operator|(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator^=(const tVector<T, L, V>& v)
    {
        set(operator^(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator<<=(const tVector<T, L, V>& v)
    {
        set(operator<<(v));
        return *(S*)this;
    }
    template <class V> DMC_DECL S& operator>>=(const tVector<T, L, V>& v)
    {
        set(operator>>(v));
        return *(S*)this;
    }

    // Unary and vector-vector component-wise const operations
    DMC_DECL S abs() const
    {
        const T* tp = data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = ::abs(tp[i]);
        return r;
    }
    template <class V> DMC_DECL S min(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = ::min(tp[i], vp[i]); // Use min defined with above macro
        return r;
    }
    template <class V> DMC_DECL S max(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = ::max(tp[i], vp[i]); // Use max defined with above macro
        return r;
    }
    template <class V, class W> DMC_DECL [[nodiscard]] S clamp(const tVector<T, L, V>& lo, const tVector<T, L, W>& hi) const
    {
        const T* tp = data();
        const T* lop = lo.data();
        const T* hip = hi.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = ::clamp(tp[i], lop[i], hip[i]);
        return r;
    }
    template <class V> DMC_DECL S operator+(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] + vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator-(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] - vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator*(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] * vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator/(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] / vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator%(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] % vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator&(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] & vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator|(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] | vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator^(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] ^ vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator<<(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] << vp[i];
        return r;
    }
    template <class V> DMC_DECL S operator>>(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        S r;
        T* rp = r.data();
        for (int i = 0; i < L; i++) rp[i] = tp[i] >> vp[i];
        return r;
    }

    // Vector-vector const operations
    template <class V> DMC_DECL T dot(const tVector<T, L, V>& v) const
    {
        const T* tp = data();
        const T* vp = v.data();
        T r = (T)0;
        for (int i = 0; i < L; i++) r += tp[i] * vp[i];
        return r;
    }
};
struct int3 {
    int x, y, z;
};

class i3vec : public tVector<int, 3, i3vec>, public int3 {
public:
    DMC_DECL i3vec() { setZero(); }
    DMC_DECL i3vec(int a) { set(a); }
    DMC_DECL i3vec(int xx, int yy, int zz)
    {
        x = xx;
        y = yy;
        z = zz;
    }

    DMC_DECL const int* data() const { return &x; }
    DMC_DECL int* data() { return &x; }
    static DMC_DECL i3vec fromPtr(const int* ptr) { return i3vec(ptr[0], ptr[1], ptr[2]); }

    template <class V> DMC_DECL i3vec(const tVector<int, 3, V>& v) { set(v); }
    template <class V> DMC_DECL i3vec& operator=(const tVector<int, 3, V>& v)
    {
        set(v);
        return *this;
    }
};

typedef enum { CURVE_MORTON, CURVE_HILBERT, CURVE_RASTER, CURVE_BOUSTRO, CURVE_TILED2, CURVE_COUNT } SFCurveType;
static char const* SFCurveNames[] = {"CURVE_MORTON", "CURVE_HILBERT", "CURVE_RASTER", "CURVE_BOUSTRO", "CURVE_TILED2", "NONE"};

// Public, generic interfaces
// Statically choose the SFCurveType (performant)
template <typename intcode_t, SFCurveType CT> DMC_DECL intcode_t toSFCurveCode(i3vec x) {}
template <typename intcode_t, SFCurveType CT> DMC_DECL i3vec toSFCurveCoords(intcode_t x) {}

// Dynamically choose the SFCurveType
template <typename intcode_t> DMC_DECL intcode_t toSFCurveCode(i3vec x, SFCurveType CT)
{
    switch (CT) {
    case CURVE_MORTON: return toSFCurveCode<intcode_t, CURVE_MORTON>(x);
    case CURVE_HILBERT: return toSFCurveCode<intcode_t, CURVE_HILBERT>(x);
    case CURVE_RASTER: return toSFCurveCode<intcode_t, CURVE_RASTER>(x);
    case CURVE_BOUSTRO: return toSFCurveCode<intcode_t, CURVE_BOUSTRO>(x);
    case CURVE_TILED2: return toSFCurveCode<intcode_t, CURVE_TILED2>(x);
    case CURVE_COUNT: break; // quiet warning
    }
    return 0;
}
template <typename intcode_t> DMC_DECL i3vec toSFCurveCoords(intcode_t x, SFCurveType CT)
{
    switch (CT) {
    case CURVE_MORTON: return toSFCurveCoords<intcode_t, CURVE_MORTON>(x);
    case CURVE_HILBERT: return toSFCurveCoords<intcode_t, CURVE_HILBERT>(x);
    case CURVE_RASTER: return toSFCurveCoords<intcode_t, CURVE_RASTER>(x);
    case CURVE_BOUSTRO: return toSFCurveCoords<intcode_t, CURVE_BOUSTRO>(x);
    case CURVE_TILED2: return toSFCurveCoords<intcode_t, CURVE_TILED2>(x);
    case CURVE_COUNT: break; // quiet warning
    }
    return i3vec(0);
}

// The curve order (number of bits in each dimension)
// Was sizeof(intcode_t) > sizeof(uint32_t) ? 20 : 10
template <typename intcode_t> DMC_DECL constexpr const int curveOrder() { return (sizeof(intcode_t) * 8) / 3; }

// Declarations of actual implementations
template <typename intcode_t> DMC_DECL intcode_t toMortonCode(i3vec v);
template <typename intcode_t> DMC_DECL intcode_t toHilbertCode(i3vec v);
template <typename intcode_t> DMC_DECL intcode_t toRasterCode(i3vec v);
template <typename intcode_t> DMC_DECL intcode_t toBoustroCode(i3vec v);
template <typename intcode_t> DMC_DECL intcode_t toTiled2Code(i3vec v);

template <typename intcode_t> DMC_DECL i3vec toMortonCoords(const intcode_t p);
template <typename intcode_t> DMC_DECL i3vec toHilbertCoords(const intcode_t p);
template <typename intcode_t> DMC_DECL i3vec toRasterCoords(const intcode_t p);
template <typename intcode_t> DMC_DECL i3vec toBoustroCoords(const intcode_t p);
template <typename intcode_t> DMC_DECL i3vec toTiled2Coords(const intcode_t p);

// Convert from two template arguments to one
template <> DMC_DECL uint32_t toSFCurveCode<uint32_t, CURVE_MORTON>(i3vec v) { return toMortonCode<uint32_t>(v); }
template <> DMC_DECL uint32_t toSFCurveCode<uint32_t, CURVE_HILBERT>(i3vec v) { return toHilbertCode<uint32_t>(v); }
template <> DMC_DECL uint32_t toSFCurveCode<uint32_t, CURVE_RASTER>(i3vec v) { return toRasterCode<uint32_t>(v); }
template <> DMC_DECL uint32_t toSFCurveCode<uint32_t, CURVE_BOUSTRO>(i3vec v) { return toBoustroCode<uint32_t>(v); }
template <> DMC_DECL uint32_t toSFCurveCode<uint32_t, CURVE_TILED2>(i3vec v) { return toTiled2Code<uint32_t>(v); }

template <> DMC_DECL uint64_t toSFCurveCode<uint64_t, CURVE_MORTON>(i3vec v) { return toMortonCode<uint64_t>(v); }
template <> DMC_DECL uint64_t toSFCurveCode<uint64_t, CURVE_HILBERT>(i3vec v) { return toHilbertCode<uint64_t>(v); }
template <> DMC_DECL uint64_t toSFCurveCode<uint64_t, CURVE_RASTER>(i3vec v) { return toRasterCode<uint64_t>(v); }
template <> DMC_DECL uint64_t toSFCurveCode<uint64_t, CURVE_BOUSTRO>(i3vec v) { return toBoustroCode<uint64_t>(v); }
template <> DMC_DECL uint64_t toSFCurveCode<uint64_t, CURVE_TILED2>(i3vec v) { return toTiled2Code<uint64_t>(v); }

template <> DMC_DECL i3vec toSFCurveCoords<uint32_t, CURVE_MORTON>(const uint32_t p) { return toMortonCoords<uint32_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint32_t, CURVE_HILBERT>(const uint32_t p) { return toHilbertCoords<uint32_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint32_t, CURVE_RASTER>(const uint32_t p) { return toRasterCoords<uint32_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint32_t, CURVE_BOUSTRO>(const uint32_t p) { return toBoustroCoords<uint32_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint32_t, CURVE_TILED2>(const uint32_t p) { return toTiled2Coords<uint32_t>(p); }

template <> DMC_DECL i3vec toSFCurveCoords<uint64_t, CURVE_MORTON>(const uint64_t p) { return toMortonCoords<uint64_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint64_t, CURVE_HILBERT>(const uint64_t p) { return toHilbertCoords<uint64_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint64_t, CURVE_RASTER>(const uint64_t p) { return toRasterCoords<uint64_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint64_t, CURVE_BOUSTRO>(const uint64_t p) { return toBoustroCoords<uint64_t>(p); }
template <> DMC_DECL i3vec toSFCurveCoords<uint64_t, CURVE_TILED2>(const uint64_t p) { return toTiled2Coords<uint64_t>(p); }

// Actual implementations
template <typename intcode_t> DMC_DECL intcode_t toMortonCode(i3vec v_)
{
    const int nbits = curveOrder<intcode_t>();
    i3vec v = v_;

    // This is because explicit template specialization for the fast types didn't work.
    if constexpr (nbits == 10) {
        v.x = (v.x | (v.x << 16)) & 0x030000FF;
        v.x = (v.x | (v.x << 8)) & 0x0300F00F;
        v.x = (v.x | (v.x << 4)) & 0x030C30C3;
        v.x = (v.x | (v.x << 2)) & 0x09249249;

        v.y = (v.y | (v.y << 16)) & 0x030000FF;
        v.y = (v.y | (v.y << 8)) & 0x0300F00F;
        v.y = (v.y | (v.y << 4)) & 0x030C30C3;
        v.y = (v.y | (v.y << 2)) & 0x09249249;

        v.z = (v.z | (v.z << 16)) & 0x030000FF;
        v.z = (v.z | (v.z << 8)) & 0x0300F00F;
        v.z = (v.z | (v.z << 4)) & 0x030C30C3;
        v.z = (v.z | (v.z << 2)) & 0x09249249;

        return (v.z << 2) | (v.y << 1) | v.x;
    } else if constexpr (nbits == 21) {
        uint64_t x = v.x, y = v.y, z = v.z;

        x = (x | (x << 32)) & 0x001F00000000FFFF;
        x = (x | (x << 16)) & 0x001F0000FF0000FF;
        x = (x | (x << 8)) & 0x100F00F00F00F00F;
        x = (x | (x << 4)) & 0x10C30C30C30C30C3;
        x = (x | (x << 2)) & 0x1249249249249249;

        y = (y | (y << 32)) & 0x001F00000000FFFF;
        y = (y | (y << 16)) & 0x001F0000FF0000FF;
        y = (y | (y << 8)) & 0x100F00F00F00F00F;
        y = (y | (y << 4)) & 0x10C30C30C30C30C3;
        y = (y | (y << 2)) & 0x1249249249249249;

        z = (z | (z << 32)) & 0x001F00000000FFFF;
        z = (z | (z << 16)) & 0x001F0000FF0000FF;
        z = (z | (z << 8)) & 0x100F00F00F00F00F;
        z = (z | (z << 4)) & 0x10C30C30C30C30C3;
        z = (z | (z << 2)) & 0x1249249249249249;

        return (z << 2) | (y << 1) | x;
    } else {
        intcode_t x = v.x, y = v.y, z = v.z;
        intcode_t codex = 0, codey = 0, codez = 0;

        const int nbits2 = 2 * nbits;

        for (int i = 0, andbit = 1; i < nbits2; i += 2, andbit <<= 1) {
            codex |= (intcode_t)(x & andbit) << i;
            codey |= (intcode_t)(y & andbit) << i;
            codez |= (intcode_t)(z & andbit) << i;
        }

        return (codez << 2) | (codey << 1) | codex;
    }
}

// Hilbert coordinate transpose functions by John Skilling. Public domain.
// See https://stackoverflow.com/questions/499166/mapping-n-dimensional-value-to-a-point-on-hilbert-curve
// The uint32_t is the N-D coordinate type. Could be something else.

DMC_DECL void transposeFromHilbertCoords(uint32_t* X, int nbits, int dim)
{
    uint32_t N = 2 << (nbits - 1), P, Q, t;

    // Gray decode by H ^ (H/2)
    t = X[dim - 1] >> 1;
    // Corrected error in Skilling's paper on the following line. The appendix had i >= 0 leading to negative array index.
    for (int i = dim - 1; i > 0; i--) X[i] ^= X[i - 1];
    X[0] ^= t;

    // Undo excess work
    for (Q = 2; Q != N; Q <<= 1) {
        P = Q - 1;
        for (int i = dim - 1; i >= 0; i--)
            if (X[i] & Q) // Invert
                X[0] ^= P;
            else { // Exchange
                t = (X[0] ^ X[i]) & P;
                X[0] ^= t;
                X[i] ^= t;
            }
    }
}

DMC_DECL void transposeToHilbertCoords(uint32_t* X, int nbits, int dim)
{
    uint32_t M = 1 << (nbits - 1), P, Q, t;

    // Inverse undo
    for (Q = M; Q > 1; Q >>= 1) {
        P = Q - 1;
        for (int i = 0; i < dim; i++)
            if (X[i] & Q) // Invert
                X[0] ^= P;
            else { // Exchange
                t = (X[0] ^ X[i]) & P;
                X[0] ^= t;
                X[i] ^= t;
            }
    }

    // Gray encode
    for (int i = 1; i < dim; i++) X[i] ^= X[i - 1];
    t = 0;
    for (Q = M; Q > 1; Q >>= 1)
        if (X[dim - 1] & Q) t ^= Q - 1;
    for (int i = 0; i < dim; i++) X[i] ^= t;
}

template <typename intcode_t> DMC_DECL intcode_t toHilbertCode(i3vec v)
{
    const int nbits = curveOrder<intcode_t>();

    transposeToHilbertCoords((uint32_t*)v.data(), nbits, 3);
    std::swap(v.x, v.z);
    intcode_t s = toMortonCode<intcode_t>(v);

    return s;
}

template <typename intcode_t> DMC_DECL intcode_t toRasterCode(i3vec v)
{
    const int nbits = curveOrder<intcode_t>();
    intcode_t s = ((intcode_t)v.z << (intcode_t)(2 * nbits)) | ((intcode_t)v.y << (intcode_t)nbits) | (intcode_t)v.x;

    return s;
}

template <typename intcode_t> DMC_DECL intcode_t toBoustroCode(i3vec v)
{
    const int nbits = curveOrder<intcode_t>();
    const intcode_t nbitmask = ((intcode_t)1 << (intcode_t)nbits) - (intcode_t)1;

    if (v.y & 1) v.x = nbitmask & ~v.x;
    if (v.z & 1) v.y = nbitmask & ~v.y;
    if (v.z & 1) v.x = nbitmask & ~v.x;

    intcode_t s = ((intcode_t)v.z << (intcode_t)(2 * nbits)) | ((intcode_t)v.y << (intcode_t)nbits) | (intcode_t)v.x;

    return s;
}

template <typename intcode_t> DMC_DECL intcode_t toTiled2Code(i3vec v)
{
    const int nbits = curveOrder<intcode_t>();

    const int nbitsl = nbits / 2;
    const intcode_t nbitmaskl = ((intcode_t)1 << (intcode_t)nbitsl) - (intcode_t)1;
    i3vec vlo(nbitmaskl & v.x, nbitmaskl & v.y, nbitmaskl & v.z);
    intcode_t lo = ((intcode_t)vlo.z << (intcode_t)(2 * nbitsl)) | ((intcode_t)vlo.y << (intcode_t)nbitsl) | (intcode_t)vlo.x;

    const int nbitsh = nbits - nbitsl;
    const intcode_t nbitmaskh = ((intcode_t)1 << (intcode_t)nbitsh) - (intcode_t)1;
    i3vec vhi(nbitmaskh & (v.x >> nbitsl), nbitmaskh & (v.y >> nbitsl), nbitmaskh & (v.z >> nbitsl));
    intcode_t hi = ((intcode_t)vhi.z << (intcode_t)(2 * nbitsh)) | ((intcode_t)vhi.y << (intcode_t)nbitsh) | (intcode_t)vhi.x;

    return (hi << (intcode_t)(nbitsl * 3)) | lo;
}

template <typename intcode_t> DMC_DECL i3vec toMortonCoords(intcode_t p)
{
    // From https://github.com/Forceflow/libmorton/blob/main/include/libmorton/morton3D.h
    const unsigned int nbits = curveOrder<intcode_t>();
    i3vec v(0);
    for (unsigned int i = 0; i <= nbits; ++i) {
        intcode_t selector = 1;
        unsigned int shift_selector = 3 * i;
        unsigned int shiftback = 2 * i;
        v.x |= (p & (selector << shift_selector)) >> (shiftback);
        v.y |= (p & (selector << (shift_selector + 1))) >> (shiftback + 1);
        v.z |= (p & (selector << (shift_selector + 2))) >> (shiftback + 2);
    }
    return v;
}

template <typename intcode_t> DMC_DECL i3vec toHilbertCoords(intcode_t p)
{
    const int nbits = curveOrder<intcode_t>();

    i3vec v = toMortonCoords<intcode_t>(p);
    std::swap(v.x, v.z);
    transposeFromHilbertCoords((uint32_t*)v.data(), nbits, 3);

    return v;
}

template <typename intcode_t> DMC_DECL i3vec toRasterCoords(intcode_t p)
{
    const int nbits = curveOrder<intcode_t>();
    const intcode_t nbitmask = ((intcode_t)1 << (intcode_t)nbits) - (intcode_t)1;

    i3vec v((int)(p & nbitmask), (int)((p >> (intcode_t)nbits) & nbitmask), (int)((p >> (intcode_t)(2 * nbits)) & nbitmask));

    return v;
}

template <typename intcode_t> DMC_DECL i3vec toBoustroCoords(intcode_t p)
{
    const int nbits = curveOrder<intcode_t>();
    const intcode_t nbitmask = ((intcode_t)1 << (intcode_t)nbits) - (intcode_t)1;

    i3vec v((int)(p & nbitmask), (int)((p >> (intcode_t)nbits) & nbitmask), (int)((p >> (intcode_t)(2 * nbits)) & nbitmask));

    if (v.z & 1) v.y = nbitmask & ~v.y;
    if (v.z & 1) v.x = nbitmask & ~v.x;
    if (v.y & 1) v.x = nbitmask & ~v.x;

    return v;
}

template <typename intcode_t> DMC_DECL i3vec toTiled2Coords(intcode_t p)
{
    const int nbits = curveOrder<intcode_t>();

    const int nbitsl = nbits / 2;
    const intcode_t nbitmaskl = ((intcode_t)1 << (intcode_t)nbitsl) - (intcode_t)1;

    const int nbitsh = nbits - nbitsl;
    const intcode_t nbitmaskh = ((intcode_t)1 << (intcode_t)nbitsh) - (intcode_t)1;

    intcode_t hi = p >> (intcode_t)(nbitsl * 3);
    intcode_t lo = p & ((intcode_t)1 << (intcode_t)(nbitsl * 3)) - (intcode_t)1;

    i3vec vlo((int)(lo & nbitmaskl), (int)((lo >> (intcode_t)nbitsl) & nbitmaskl), (int)((lo >> (intcode_t)(2 * nbitsl)) & nbitmaskl));
    i3vec vhi((int)(hi & nbitmaskh), (int)((hi >> (intcode_t)nbitsh) & nbitmaskh), (int)((hi >> (intcode_t)(2 * nbitsh)) & nbitmaskh));

    i3vec v;
    v.x = (vhi.x << nbitsl) | vlo.x;
    v.y = (vhi.y << nbitsl) | vlo.y;
    v.z = (vhi.z << nbitsl) | vlo.z;

    return v;
}

#if 0
// Convert a 3D float vector to 3 integer code for later conversion to Morton
template <typename Vec_T> class FixedPointifier {
public:
    FixedPointifier(const tAABB<Vec_T>& world, const uint32_t nbits_) : m_nbits(nbits_)
    {
        const float keyMaxf = (float)((1 << m_nbits) - 1);

        m_scale = keyMaxf / world.extent();
        m_scaleInv = world.extent() / keyMaxf;
        m_bias = world.lo();
    }

    i3vec floatToFixed(const Vec_T& P)
    {
        const int keyMaxi = (1 << m_nbits) - 1;

        Vec_T v = m_scale * (P - m_bias);
        i3vec key = v;
        key = clamp(key, i3vec(0), i3vec(keyMaxi)); // Key.clamp(i3vec(0), i3vec(keyMaxi)) is const and doesn't do what we want.

        return key;
    }

    Vec_T fixedToFloat(const i3vec& P)
    {
        const float keyMaxf = (float)((1 << m_nbits) - 1);
        Vec_T v = m_bias + f3vec(P) * m_scaleInv;

        return v;
    }

private:
    const uint32_t m_nbits;
    Vec_T m_scale, m_scaleInv;
    Vec_T m_bias;
};

#endif
