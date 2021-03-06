/* Copyright (c) 2014-2015, Stanislaw Halik <sthalik@misaki.pl>

 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#pragma once
#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <cmath>

namespace {
    // last param to fool SFINAE into overloading
    template<int i, int j, int ignored>
    struct equals
    {
        enum { value = i == j };
    };
    template<int i, int j, int min>
    struct maybe_add_swizzle
    {
        enum { value = (i == 1 || j == 1) && (i >= min || j >= min) };
    };
    template<int i1, int j1, int i2, int j2>
    struct is_vector_pair
    {
        enum { value = (i1 == i2 && j1 == 1 && j2 == 1) || (j1 == j2 && i1 == 1 && i2 == 1) };
    };
    template<int i, int j>
    struct vector_len
    {
        enum { value = i > j ? i : j };
    };
    template<int a, int b, int c, int d>
    struct is_dim3
    {
        enum { value = (a == 1 && c == 1 && b == 3 && d == 3) || (a == 3 && c == 3 && b == 1 && d == 1) };
        enum { P = a == 1 ? 1 : 3 };
        enum { Q = a == 1 ? 3 : 1 };
    };

    template<typename num, int h, int w, typename...ts>
    struct is_arglist_correct
    {
        enum { value = h * w == sizeof...(ts) };
    };
}

template<typename num, int h_, int w_>
class Mat
{
#ifdef __GNUC__
    __restrict
#endif
    num data[h_][w_];

    static_assert(h_ > 0 && w_ > 0, "must have positive mat dimensions");

    Mat(std::initializer_list<num>&& xs) = delete;

public:

    // parameters w_ and h_ are rebound so that SFINAE occurs
    // removing them causes a compile-time error -sh 20150811
    
    template<int Q = w_> typename std::enable_if<equals<Q, 1, 0>::value, num>::type
    inline operator()(int i) const { return data[i][0]; }
    
    template<int P = h_> typename std::enable_if<equals<P, 1, 1>::value, num>::type
    inline operator()(int i) const { return data[0][i]; }
    
    template<int Q = w_> typename std::enable_if<equals<Q, 1, 2>::value, num&>::type
    inline operator()(int i) { return data[i][0]; }
    
    template<int P = h_> typename std::enable_if<equals<P, 1, 3>::value, num&>::type
    inline operator()(int i) { return data[0][i]; }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 1>::value, num>::type
    inline x() const { return operator()(0); }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 2>::value, num>::type
    inline y() const { return operator()(1); }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 3>::value, num>::type
    inline z() const { return operator()(2); }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 4>::value, num>::type
    inline w() const { return operator()(3); }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 1>::value, num&>::type
    inline x() { return operator()(0); }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 2>::value, num&>::type
    inline y() { return operator()(1); }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 3>::value, num&>::type
    inline z() { return operator()(2); }
    
    template<int P = h_, int Q = w_> typename std::enable_if<maybe_add_swizzle<P, Q, 4>::value, num&>::type
    inline w() { return operator()(3); }
    
    template<int R, int S, int P = h_, int Q = w_>
    typename std::enable_if<is_vector_pair<R, S, P, Q>::value, num>::type
    dot(const Mat<num, R, S>& p2) const {
        num ret = 0;
        constexpr int len = vector_len<R, S>::value;
        for (int i = 0; i < len; i++)
            ret += operator()(i) * p2(i);
        return ret;
    }
    
    template<int R, int S, int P = h_, int Q = w_>
    typename std::enable_if<is_dim3<P, Q, R, S>::value, Mat<num, is_dim3<P, Q, R, S>::P, is_dim3<P, Q, R, S>::Q>>::type
    cross(const Mat<num, R, S>& p2) const
    {
        return Mat<num, R, S>(y() * p2.z() - p2.y() * z(),
                              p2.x() * z() - x() * p2.z(),
                              x() * p2.y() - y() * p2.x());
    }
    
    Mat<num, h_, w_> operator+(const Mat<num, h_, w_>& other) const
    {
        Mat<num, h_, w_> ret;
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                ret(j, i) = data[j][i] + other.data[j][i];
        return ret;
    }
    
    Mat<num, h_, w_> operator-(const Mat<num, h_, w_>& other) const
    {
        Mat<num, h_, w_> ret;
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                ret(j, i) = data[j][i] - other.data[j][i];
        return ret;
    }
    
    Mat<num, h_, w_> operator+(const num& other) const
    {
        Mat<num, h_, w_> ret;
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                ret(j, i) = data[j][i] + other;
        return ret;
    }
    
    Mat<num, h_, w_> operator-(const num& other) const
    {
        Mat<num, h_, w_> ret;
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                ret(j, i) = data[j][i] - other;
        return ret;
    }
    
    Mat<num, h_, w_> operator*(const num other) const
    {
        Mat<num, h_, w_> ret;
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                ret(j, i) = data[j][i] * other;
        return ret;
    }
    
    template<int p>
    Mat<num, h_, p> operator*(const Mat<num, w_, p>& other) const
    {
        Mat<num, h_, p> ret;
        for (int k = 0; k < h_; k++)
            for (int i = 0; i < p; i++)
            {
                ret(k, i) = 0;
                for (int j = 0; j < w_; j++)
                    ret(k, i) += data[k][j] * other(j, i);
            }
        return ret;
    }

    inline num operator()(int j, int i) const { return data[j][i]; }
    inline num& operator()(int j, int i) { return data[j][i]; }

    template<typename... ts, int h__ = h_, int w__ = w_,
             typename = typename std::enable_if<is_arglist_correct<num, h__, w__, ts...>::value>::type>
    Mat(const ts... xs)
    {
        const std::initializer_list<num> init = { static_cast<num>(xs)... };
        auto iter = init.begin();
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                data[j][i] = *iter++;
    }

    Mat()
    {
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                data[j][i] = num(0);
    }

    Mat(const num* mem)
    {
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                data[j][i] = mem[i*h_+j];
    }

    Mat(num* mem) : Mat(const_cast<const num*>(mem)) {}

    // XXX add more operators as needed, third-party dependencies mostly
    // not needed merely for matrix algebra -sh 20141030

    static Mat<num, h_, h_> eye()
    {
        Mat<num, h_, h_> ret;
        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                ret.data[j][i] = 0;

        for (int i = 0; i < h_; i++)
            ret.data[i][i] = 1;

        return ret;
    }

    Mat<num, w_, h_> t() const
    {
        Mat<num, w_, h_> ret;

        for (int j = 0; j < h_; j++)
            for (int i = 0; i < w_; i++)
                ret(i, j) = data[j][i];

        return ret;
    }
    
    template<int h__, int w__> using dmat = Mat<double, h__, w__>;
    
    // http://stackoverflow.com/a/18436193
    static dmat<3, 1> rmat_to_euler(const dmat<3, 3>& R)
    {
        static constexpr double pi = 3.141592653;
        const double pitch_1 = asin(-R(0, 2));
        const double pitch_2 = pi - pitch_1;
        const double cos_p1 = cos(pitch_1), cos_p2 = cos(pitch_2);
        const double roll_1 = atan2(R(1, 2) / cos_p1, R(2, 2) / cos_p1);
        const double roll_2 = atan2(R(1, 2) / cos_p2, R(2, 2) / cos_p2);
        const double yaw_1 = atan2(R(0, 1) / cos_p1, R(0, 0) / cos_p1);
        const double yaw_2 = atan2(R(0, 1) / cos_p2, R(0, 0) / cos_p2);
        if (std::abs(pitch_1) + std::abs(roll_1) + std::abs(yaw_1) > std::abs(pitch_2) + std::abs(roll_2) + std::abs(yaw_2))
        {
            bool fix_neg_pitch = pitch_1 < 0;
            return dmat<3, 1>(yaw_2, std::fmod(fix_neg_pitch ? -pi - pitch_1 : pitch_2, pi), roll_2);
        }
        else
            return dmat<3, 1>(yaw_1, pitch_1, roll_1);
    }
    
    // tait-bryan angles, not euler
    static dmat<3, 3> euler_to_rmat(const double* input)
    {
        static constexpr double pi = 3.141592653;
        auto H = input[0] * pi / 180;
        auto P = input[1] * pi / 180;
        auto B = input[2] * pi / 180;
    
        const auto c1 = cos(H);
        const auto s1 = sin(H);
        const auto c2 = cos(P);
        const auto s2 = sin(P);
        const auto c3 = cos(B);
        const auto s3 = sin(B);
    
        double foo[] = {
            // z
            c1 * c2,
            c1 * s2 * s3 - c3 * s1,
            s1 * s3 + c1 * c3 * s2,
            // y
            c2 * s1,
            c1 * c3 + s1 * s2 * s3,
            c3 * s1 * s2 - c1 * s3,
            // x
            -s2,
            c2 * s3,
            c2 * c3
        };
    
        return dmat<3, 3>(foo);
    }
};
   
template<int h_, int w_> using dmat = Mat<double, h_, w_>;
