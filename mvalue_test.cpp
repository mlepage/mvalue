// mvalue
// Copyright (C) 2007 Marc Lepage

#include "mvalue.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <typeinfo>
#include <vector>

#define CHECK(v, T, ...) \
{ \
    T a[] = { __VA_ARGS__ }; \
    assert(v.get_value_type() == mvalue::ftraits<T>::value_type); \
    assert(v.size() == sizeof(a)/sizeof(a[0])); \
    for (int n = 0; n != v.size(); ++n) \
    { \
        assert(v[n] == a[n]); \
    } \
}

int main(int argc, char* argv[])
{
    // Let's work with types.
    {
        // We can define variables.
        int n = 123;
        double d = 123.456;

        // We can use value types to represent the types of these variables at run time.
        mvalue::value_type n_type = mvalue::int_type;
        mvalue::value_type d_type = mvalue::double_type;

        // We can use traits to convert between fundamental types and value types at compile time.
        assert(mvalue::ftraits<int>::value_type == mvalue::int_type);
        assert(mvalue::ftraits<double>::value_type == mvalue::double_type);
        assert(typeid(mvalue::vtraits<mvalue::int_type>::fundamental_type) == typeid(int));
        assert(typeid(mvalue::vtraits<mvalue::double_type>::fundamental_type) == typeid(double));

        // An expression has its own type, the result of applying the usual arithmetic conversions.
        // We can calculate this result type automatically at compile time.
        mvalue::uac<int, double>::result_type nd = n + d;
        assert(typeid(nd) == typeid(n + d));
        mvalue::value_type nd_type = mvalue::ftraits<mvalue::uac<int, double>::result_type>::value_type;
    }

    // The nil value is empty. It has no elements. It has no type.
    mvalue v_nil;
    assert(v_nil.empty() && v_nil.size() == 0);
    assert(v_nil.get_value_type() == mvalue::nil_type);

    // We can construct simple values from single arguments of fundamental type.
    mvalue v_false(false);
    mvalue v_true(true);
    mvalue v_char('A');
    mvalue v_int(100);
    mvalue v_ulong(100ul);
    mvalue v_float(22.0f/7.0f);
    mvalue v_double(22.0/7.0);

    // These single values will infer the appropriate type.
    assert(v_false.size() == 1 && v_false.get_value_type() == mvalue::bool_type);
    assert(v_true.size() == 1 && v_true.get_value_type() == mvalue::bool_type);
    assert(v_char.size() == 1 && v_char.get_value_type() == mvalue::char_type);
    assert(v_int.size() == 1 && v_int.get_value_type() == mvalue::int_type);
    assert(v_ulong.size() == 1 && v_ulong.get_value_type() == mvalue::ulong_type);
    assert(v_float.size() == 1 && v_float.get_value_type() == mvalue::float_type);
    assert(v_double.size() == 1 && v_double.get_value_type() == mvalue::double_type);

    // We can perform many operations on these values, as if they were regular variables.
    assert(v_false != v_true);
    assert(v_char == v_char);
    assert(v_int == v_ulong);
    assert(v_float != v_double);
    assert(v_false < v_true);
    assert(v_char < v_int);
    assert(v_float < v_double);

    int a_int[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    size_t a_int_sz = sizeof(a_int)/sizeof(a_int[0]);
    double a_double[] = { 0.0, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9 };
    size_t a_double_sz = sizeof(a_double)/sizeof(a_double[0]);

    {
        std::vector<double> vdbl(a_int, a_int + a_int_sz);
        mvalue v_int(a_int, a_int + a_int_sz);
        mvalue v_double(a_double, a_double + a_double_sz);

        assert(v_int.get_value_type() == mvalue::int_type);
        assert(v_double.get_value_type() == mvalue::double_type);
        assert(v_int != v_double);

        v_double.set_value_type(mvalue::int_type);
        v_double.set_value_type(mvalue::double_type);
        assert(v_int == v_double);
    }

    {
        mvalue vi(mvalue::int_type, 4);
        vi[0] = 1;
        vi[1] = 2;
        vi[2] = 3;
        vi[3] = 4;
        mvalue vd(mvalue::double_type, 4);
        vd[0] = 1;
        vd[1] = 2;
        vd[2] = 3;
        vd[3] = 4;
        assert(vi == vd);
        vi[2] = 2;
        assert(vi < vd);
// TODO this line doesn't compile under VC9
//      assert(std::lexicographical_compare(vi.begin(), vi.end(), vd.begin(), vd.end()));
        assert(!std::lexicographical_compare(vi.begin(), vi.end(), vd.begin(), vd.end(), std::greater<double>()));
    }

    {
        mvalue vi(mvalue::int_type, 10);
        for (int n = 0; n != vi.size(); ++n)
        {
            vi[n] = n;
        }

        mvalue vcopy(vi.begin() + 3, vi.begin() + 8);
        for (int n = 0; n != vcopy.size(); ++n)
        {
            std::cout << (int)vcopy[n] << '\n';
        }

        vcopy.resize(3);
        for (int n = 0; n != vcopy.size(); ++n)
        {
            std::cout << (int)vcopy[n] << '\n';
        }
        vcopy.resize(5, 10.4);
        for (int n = 0; n != vcopy.size(); ++n)
        {
            std::cout << (int)vcopy[n] << '\n';
        }
    }

    {
        // Test iterators.
        mvalue v(a_int, a_int + 10);
        CHECK(v, int, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

        int n;
        mvalue::iterator it;
        mvalue::reverse_iterator rit;

        for (it = v.begin(), n = 0; it != v.end(); ++it, ++n)
        {
            assert(*it == n);
            assert(it == v.begin() + n);
//          assert(it == v.begin()[n]);
            assert(*it == *(v.begin() + n));
        }
        assert(it == v.end() && n == 10);

        do
        {
            --it, --n;
            assert(*it == n);
            assert(it == v.begin() + n);
//          assert(it == v.begin()[n]);
            assert(*it == *(v.begin() + n));
        } while (it != v.begin());
        assert(it == v.begin() && n == 0);

        // TODO need to test rbegin, rend, reverse iterators
        // best to look this up in stroustrup
    }

    {
        // Test size.
        mvalue v_nil;
        assert(v_nil.size() == 0);

        mvalue v_int(mvalue::int_type);
        assert(v_int.size() == 0);

        for (int n = 0; n != 10; ++n)
        {
            mvalue v_int(mvalue::int_type, n);
            assert(v_int.size() == n);
        }
    }

    {
        // Test resize.

        mvalue v(a_int, a_int + 3);
        CHECK(v, int, 0, 1, 2);

        v.resize(5);
        CHECK(v, int, 0, 1, 2, 0, 0);

        v.resize(7, -1);
        CHECK(v, int, 0, 1, 2, 0, 0, -1, -1);

        v.resize(7, -2);
        CHECK(v, int, 0, 1, 2, 0, 0, -1, -1);

        v.resize(4, -3);
        CHECK(v, int, 0, 1, 2, 0);

        v.resize(2);
        CHECK(v, int, 0, 1);

        v.resize(5, 3.14); // trunc
        CHECK(v, int, 0, 1, 3, 3, 3);
    }

    {
        // Test empty.
        mvalue v_nil;
        assert(v_nil.empty());

        mvalue v_false(false);
        assert(!v_false.empty());
        mvalue v_char('A');
        assert(!v_char.empty());

        mvalue v_uchar(mvalue::uchar_type);
        assert(v_uchar.empty());
        signed char* p = 0;
        mvalue v_schar(p, p);
        assert(v_schar.empty());

        mvalue v_int(a_int, a_int + 2);
        assert(!v_int.empty());
        v_int.erase(v_int.begin());
        assert(!v_int.empty());
        v_int.erase(v_int.begin());
        assert(v_int.empty());

        mvalue v_double(a_double, a_double + 10);
        assert(!v_double.empty());
        v_double.clear();
        assert(v_double.empty());
    }

    {
        // Test element access.
        mvalue v(a_int, a_int + 10);
        CHECK(v, int, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

        for (int n = 0; n != 10; ++n)
        {
            assert(v[n] == n);
            assert(v.at(n) == n);
        }
        assert(v.front() == 0);
        assert(v.back() == 9);
    }

    {
        // Test insert.
        mvalue v(a_int, a_int + 3);
        CHECK(v, int, 0, 1, 2);
        mvalue::iterator it;

        // insert { 10 }
        it = v.insert(v.begin() + 1, 10);
        CHECK(v, int, 0, 10, 1, 2);
        assert(it == v.begin() + 1);

        // insert { 15.5 }
        it = v.insert(v.begin() + 3, 15.5);
        CHECK(v, int, 0, 10, 1, 15, 2);
        assert(it == v.begin() + 3);

        // insert { 'A', 'A', 'A' }
        v.insert(v.begin() + 4, 3, 'A');
        CHECK(v, int, 0, 10, 1, 15, 'A', 'A', 'A', 2);

        // insert { 3.3, 4.4 }
        v.insert(v.begin() + 5, a_double + 3, a_double + 5); // trunc
        CHECK(v, int, 0, 10, 1, 15, 'A', 3, 4, 'A', 'A', 2);

        // insert { 10, 1 }
        v.insert(v.begin() + 8, v.begin() + 1, v.begin() + 3);
        CHECK(v, int, 0, 10, 1, 15, 'A', 3, 4, 'A', 10, 1, 'A', 2);
    }

    {
        // Test erase.
        mvalue v(a_int, a_int + 10);
        CHECK(v, int, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
        mvalue::iterator it;

        // erase { 0 }
        it = v.erase(v.begin());
        CHECK(v, int, 1, 2, 3, 4, 5, 6, 7, 8, 9);
        assert(it == v.begin());

        // erase { 9 }
        it = v.erase(--v.end());
        CHECK(v, int, 1, 2, 3, 4, 5, 6, 7, 8);
        assert(it == v.end());

        // erase { 4 }
        it = v.erase(v.begin() + 3);
        CHECK(v, int, 1, 2, 3, 5, 6, 7, 8);
        assert(it == v.begin() + 3);

        // erase { 1, 2 }
        it = v.erase(v.begin(), v.begin() + 2);
        CHECK(v, int, 3, 5, 6, 7, 8);
        assert(it == v.begin());

        // erase { 5, 6 }
        it = v.erase(v.begin() + 1, v.begin() + 3);
        CHECK(v, int, 3, 7, 8);
        assert(it == v.begin() + 1);

        // erase { 7, 8 }
        it = v.erase(----v.end(), v.end());
        CHECK(v, int, 3);
        assert(it == v.end());
    }

    {
        // Test swap.

        // Swap false and true.
        mvalue v_false(false);
        mvalue v_true(true);
        CHECK(v_false, bool, false);
        CHECK(v_true, bool, true);
        v_false.swap(v_true);
        CHECK(v_false, bool, true);
        CHECK(v_true, bool, false);
        std::swap(v_false, v_true);
        CHECK(v_false, bool, false);
        CHECK(v_true, bool, true);

        // Swap nil and multi value.
        mvalue v_nil;
        mvalue v_double(a_double, a_double + 4);
        assert(v_nil.get_value_type() == mvalue::nil_type && v_nil.empty());
        CHECK(v_double, double, 0.0, 1.1, 2.2, 3.3);
        v_nil.swap(v_double);
        CHECK(v_nil, double, 0.0, 1.1, 2.2, 3.3);
        assert(v_double.get_value_type() == mvalue::nil_type && v_double.empty());
        std::swap(v_nil, v_double);
        assert(v_nil.get_value_type() == mvalue::nil_type && v_nil.empty());
        CHECK(v_double, double, 0.0, 1.1, 2.2, 3.3);
    }

    {
        // Test clear.
        mvalue v_nil;
        v_nil.clear();
        assert(v_nil.get_value_type() == mvalue::nil_type && v_nil.empty());
        mvalue v_double(a_double, a_double + 10);
        v_double.clear();
        assert(v_double.get_value_type() == mvalue::double_type && v_double.empty());
    }

    {
        int n = 100;
        double d = 3.14;
        n += d;
        int n2 = n;
    }
    {
        mvalue v1(100);
        mvalue v2(3.14);
        v1 += v2;
        int n = v1[0];
    }
    {
        mvalue v1(mvalue::int_type, 2);
        v1[0] = 100;
        v1[1] = 200;
        mvalue v2(mvalue::double_type, 2);
        v2[0] = 3.14;
        v2[1] = 5.5;
        v1 += v2;
        int n = v1[0];
        n = v1[1];
    }
    {
        // emulate set_value_type by doing copy and swap
        mvalue v1(mvalue::double_type, 2);
        v1[0] = 1.2;
        v1[1] = 3.4;
        {
            mvalue tmp(mvalue::int_type, v1.size());
            std::copy(v1.begin(), v1.end(), tmp.begin());
            v1.swap(tmp);
        }
        double f0 = v1[0];
        double f1 = v1[1];
        assert(f0 == 1);
        assert(f1 == 3);
    }

    //mvalue v(mvalue::uchar_type, 3);

    //mvalue vint(mvalue::int_type, 3);
    //mvalue vdouble(mvalue::double_type, 3);

    //int n = v[0];
    //n = vint[1];
    //n = vdouble[2];

    int dummy = argc * 2;

    {
        bool b = true;
        char c = 'A';
        int n = dummy;
        double f = 3.14;
    }

    mvalue vbool(true);
    mvalue vchar('A');
    mvalue vint(123);
    mvalue vdouble(3.14);

    int n = vdouble[0];
    double f = vdouble[0];

    vdouble[0] = 42.42;
    f = vdouble[0];

    {
        int buf[4];
        buf[0] = 100;
        buf[1] = 200;
        buf[2] = 300;
        buf[3] = 400;
    }

    {
        mvalue vlarge(mvalue::int_type, 4);
        vlarge[0] = 100;
        vlarge[1] = 200;
        vlarge[2] = 300;
        vlarge[3] = 400;
        vlarge.resize(10);
        for (int n = 0; n != vlarge.size(); ++n)
        {
            int v = vlarge[n];
        }
        vlarge.resize(3);
        for (int n = 0; n != vlarge.size(); ++n)
        {
            int v = vlarge[n];
        }
    }

    {
        mvalue vint(mvalue::int_type, 3);
        vint[0] = 100;
        vint[1] = 101;
        vint[2] = 102;
        mvalue vdbl(mvalue::double_type, 3);
        vdbl[0] = vdbl[1] = vdbl[2] = vint[0];
        double f0 = vdbl[0];
        double f1 = vdbl[1];
        double f2 = vdbl[2];

        //std::transform(vint.begin(), vint.end(), vdbl.begin(), MultValue<int>(5));
        std::transform(vint.begin(), vint.end(), vdbl.begin(), std::bind2nd(std::multiplies<int>(), 2));
        f0 = vdbl[0];
        f1 = vdbl[1];
        f2 = vdbl[2];
    }

    int fs = sizeof(long);
    int fss = sizeof(long long);
    int fsss = sizeof(long double);

    int s = sizeof(mvalue);
    int ss = sizeof(mvalue::reference);
    int sss = sizeof(mvalue::iterator);

    {
        mvalue vint(mvalue::int_type, 10);
        for (int n = 0; n != vint.size(); ++n)
        {
            vint[n] = 100 + n;
        }
        mvalue vdbl(mvalue::double_type, 10);
        std::copy(vint.begin(), vint.end(), vdbl.begin());
        for (int n = 0; n != vdbl.size(); ++n)
        {
            double test = vdbl[n];
        }
    }
    {
        int vint[10];
        for (int n = 0; n != sizeof(vint)/sizeof(vint[0]); ++n)
        {
            vint[n] = 100 + n;
        }
        double vdbl[10];
        double* dst = vdbl;
        for (int* src = vint;
            src != vint + sizeof(vint)/sizeof(vint[0]); ++src, ++dst)
        {
            *dst = *src;
        }
        for (int n = 0; n != sizeof(vdbl)/sizeof(vdbl[0]); ++n)
        {
            double test = vdbl[n];
        }
    }
    {
        mvalue vint(mvalue::int_type, 10);
        for (int n = 0; n != vint.size(); ++n)
        {
            vint[n] = 100 + n;
        }
        mvalue vdbl(mvalue::double_type, 10);
        mvalue::iterator dst = vdbl.begin();
        for (mvalue::const_iterator src = vint.begin();
            src != vint.end(); ++src, ++dst)
        {
            *dst = *src;
        }
        for (int n = 0; n != vdbl.size(); ++n)
        {
            double test = vdbl[n];
        }
    }
    {
        std::vector<int> vint(10);
        for (int n = 0; n != vint.size(); ++n)
        {
            vint[n] = 100 + n;
        }
        std::vector<double> vdbl(10);
        std::vector<double>::iterator dst = vdbl.begin();
        for (std::vector<int>::const_iterator src = vint.begin();
            src != vint.end(); ++src, ++dst)
        {
            *dst = *src;
        }
        for (int n = 0; n != vdbl.size(); ++n)
        {
            double test = vdbl[n];
        }
    }

    {
        mvalue vd(mvalue::double_type, 3);
        vd[0] = 0.01;
        vd[1] = 1.2;
        vd[2] = 3.4;
#if 0
        // The standard requires this code to work.
        double* p0 = &vd[0];
        double* p1 = &vd[1];
        double* p2 = &vd[2];
        double f0 = *p0;
        double f1 = *p1;
        double f2 = *p2;
#endif
    }

    // TODO should be able to do ++++++it

    return 0;
}
