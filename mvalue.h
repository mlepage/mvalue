// mvalue
// Copyright (C) 2007 Marc Lepage

#ifndef MVALUE_H
#define MVALUE_H

#include <algorithm>    // lexicographical_compare, etc.
#include <cassert>
#include <cstddef>      // size_t, ptrdiff_t etc.
#include <functional>   // equal_to, etc.
#include <iterator>     // iterator traits, reverse_iterator, etc.
#include <memory>       // allocators etc.
#include <stdexcept>    // out_of_range etc.
#include <utility>      // swap etc.

#ifndef MVALUE_LONGLONG_SUPPORT
#define MVALUE_LONGLONG_SUPPORT 1
#endif

/* NOTES

see also http://www.boost.org/libs/utility/Collection.html
see also http://www.gotw.ca/publications/mill09.htm
http://www.boost.org/libs/utility/operators.htm                 discussion of NVRO and other operator optimizations
http://lists.boost.org/Archives/boost/2001/09/17806.php         templates for usual arithmetic conversions
More Exceptional C++ and Effective STL talk about vector<bool> and proxied containers
More Effecitve C++ Item 30 deals with proxies

CUJ article (Alexandrescu) on discriminated unions
http://www.ddj.com/dept/cpp/184403828 

T* p = &c[0]; // this code should work for a standard container

vector supplies iterators and dynamic
deque may be more efficient (though not contiguous)
array initialization should be like valarray

see stroustrup p670 for an iterator example

Naming conventions:
T is a fundamental type (like int, double)
t is a value of T

General philosophy:
- vector<T> is statically typed
- but we want to be more dynamic
- therefore allow the usual arithmetic conversions in this container if we can

TODO:
x provide access to address of element via void*
- optionally use longlong for swap buf_
- eliminate warnings
- flesh out API
  - assignment
  - promotions
  - resizing
- io routines
- exception safety
- complexity guarantees
- go over details in standard for each function
- verify behaviour of mvalue with nil_type everywhere
- support string, char*, etc.?
- implement small string optimization, medium too
- try to make vtbl arrays constant
- figure out why empty needs to be emptyx
- templatize the util::noop overloads
- use allocators
  - hotrod memory management to be faster (pools etc.)
- write a debugger visualizer
- write a comprehensive test suite
- assertions everywhere user input comes in, and at lowest level
- test on other compilers, platforms, etc.
- performance benchmarking
- analyze disassembly
- documentation, design rationale, etc.
- usage by beta testers
- submit to boost for standardization

*/

class mvalue
{

public: // types

    // NONSTD - reference can't really be a proxy
    class reference;
    class const_reference;

    class iterator;
    class const_iterator;

    typedef size_t                                          size_type;
    typedef ptrdiff_t                                       difference_type;

    // NONSTD - value_type should be a static template
    enum value_type
    {
        bool_type, char_type, wchar_type,
        schar_type, uchar_type, short_type, ushort_type, int_type, uint_type,
        long_type, ulong_type, longlong_type, ulonglong_type,
        float_type, double_type, ldouble_type,
        nil_type
    };
    value_type get_value_type() const                       { return vt_->get_value_type(); }

    void set_value_type(value_type v)
    {
        // TODO probably more efficient way to do this
        mvalue tmp(v, size());
        std::copy(begin(), end(), tmp.begin());
        swap(tmp);
    }

    // NONSTD - no allocator_type, which should be a template parameter
    // TODO this should be private?
    //typedef typename std::allocator                           alloc;

    class pointer;                                          // TODO what is pointer?
    class const_pointer;                                    // TODO what is pointer?

    typedef std::reverse_iterator<iterator>                 reverse_iterator;
    typedef std::reverse_iterator<const_iterator>           const_reverse_iterator;

public: // traits

    // TODO would be nice to protect these against inadvertent user specialization
    template <typename T>
    struct ftraits
    {
        typedef void fundamental_type;
        static const value_type value_type = nil_type;
    };
    template <value_type V>
    struct vtraits
    {
        typedef void fundamental_type;
        static const value_type value_type = nil_type;
    };

#define MVALUE_TRAITS(T, V) \
    template <> struct ftraits<T> { typedef T fundamental_type; static const value_type value_type = V; }; \
    template <> struct vtraits<V> { typedef T fundamental_type; static const value_type value_type = V; };
    MVALUE_TRAITS(bool,                 bool_type)
    MVALUE_TRAITS(char,                 char_type)
    MVALUE_TRAITS(wchar_t,              wchar_type)
    MVALUE_TRAITS(signed char,          schar_type)
    MVALUE_TRAITS(unsigned char,        uchar_type)
    MVALUE_TRAITS(short,                short_type)
    MVALUE_TRAITS(unsigned short,       ushort_type)
    MVALUE_TRAITS(int,                  int_type)
    MVALUE_TRAITS(unsigned int,         uint_type)
    MVALUE_TRAITS(long,                 long_type)
    MVALUE_TRAITS(unsigned long,        ulong_type)
    MVALUE_TRAITS(long long,            longlong_type)
    MVALUE_TRAITS(unsigned long long,   ulonglong_type)
    MVALUE_TRAITS(float,                float_type)
    MVALUE_TRAITS(double,               double_type)
    MVALUE_TRAITS(long double,          ldouble_type)
#undef MVALUE_TRAITS

private:

    // TODO can't this be private in nested template class uac?
    // TODO can't this be simplified further using traits or templates?
    template<std::size_t N> struct sized { typedef char (&type)[N]; };
    static sized<int_type>::type        uac_select(int);
    static sized<uint_type>::type       uac_select(unsigned int);
    static sized<long_type>::type       uac_select(long);
    static sized<ulong_type>::type      uac_select(unsigned long);
    static sized<longlong_type>::type   uac_select(long long);
    static sized<ulonglong_type>::type  uac_select(unsigned long long);
    static sized<float_type>::type      uac_select(float);
    static sized<double_type>::type     uac_select(double);
    static sized<ldouble_type>::type    uac_select(long double);

public: // usual arithmetic converions

    template <typename T1, typename T2>
    struct uac
    {
        typedef vtraits<static_cast<value_type>(sizeof(uac_select(true ? T1() : T2())))>::fundamental_type result_type;
    };

    static value_type vuac(value_type v1, value_type v2)        { return uac_table[v1][v2]; }

public: // construct/copy/destroy

    mvalue() : vt_(&vtnil)                                  {}

    // vector<T>()
    explicit mvalue(value_type v) : vt_(&vtempty[v])        {} // TODO handle nil_type

    mvalue(value_type v, size_type n) : vt_(&vtempty[v])    { vt_->raw_insert(*this, 0, n); std::fill_n(begin(), n, 0); }
    // Added int version to force overload resolution
    mvalue(value_type v, int n) : vt_(&vtempty[v])          { vt_->raw_insert(*this, 0, n); std::fill_n(begin(), n, 0); }

    // NONSTD
    // TODO allow these conversions to support syntax like "mvalue v = 123;"?
#define MVALUE_CTOR(T) \
    explicit mvalue(T t) : \
        vt_(&vtsingle[ftraits<T>::value_type]) \
    { \
        *reinterpret_cast<T*>(buf_) = t; \
    }
    MVALUE_CTOR(bool)
    MVALUE_CTOR(char)
    MVALUE_CTOR(wchar_t)
    MVALUE_CTOR(signed char)
    MVALUE_CTOR(unsigned char)
    MVALUE_CTOR(short)
    MVALUE_CTOR(unsigned short)
    MVALUE_CTOR(int)
    MVALUE_CTOR(unsigned int)
    MVALUE_CTOR(long)
    MVALUE_CTOR(unsigned long)
    MVALUE_CTOR(long long)
    MVALUE_CTOR(unsigned long long)
    MVALUE_CTOR(float)
    MVALUE_CTOR(double)
    MVALUE_CTOR(long double)
#undef MVALUE_CTOR

    // vector<T>(size_type n, const T& value)
#define MVALUE_CTOR(T) \
    mvalue(size_type n, T t) : \
        vt_(&vtempty[ftraits<T>::value_type]) \
    { \
        vt_->raw_insert(*this, 0, n); \
        std::uninitialized_fill_n(static_cast<T*>(ptr()), n, t); \
    }
    MVALUE_CTOR(bool)
    MVALUE_CTOR(char)
    MVALUE_CTOR(wchar_t)
    MVALUE_CTOR(signed char)
    MVALUE_CTOR(unsigned char)
    MVALUE_CTOR(short)
    MVALUE_CTOR(unsigned short)
    MVALUE_CTOR(int)
    MVALUE_CTOR(unsigned int)
    MVALUE_CTOR(long)
    MVALUE_CTOR(unsigned long)
    MVALUE_CTOR(long long)
    MVALUE_CTOR(unsigned long long)
    MVALUE_CTOR(float)
    MVALUE_CTOR(double)
    MVALUE_CTOR(long double)
#undef MVALUE_CTOR

    template <typename InputIterator>
    mvalue(InputIterator first, InputIterator last) :
        vt_(&vtempty[ftraits<std::iterator_traits<InputIterator>::value_type>::value_type])
    {
        vt_->raw_insert(*this, 0, std::distance(first, last));
        std::copy(first, last, begin()); // TODO use memcpy?
    }
    template <>
    mvalue(const_iterator first, const_iterator last) :
        vt_(&vtempty[first.get_value_type()]) // TODO be careful of nil_type
    {
        vt_->raw_insert(*this, 0, last - first);
        std::copy(first, last, begin()); // TODO use memcpy?
    }
    template <>
    mvalue(iterator first, iterator last) :
        vt_(&vtempty[first.get_value_type()]) // TODO be careful of nil_type
    {
        vt_->raw_insert(*this, 0, last - first);
        std::copy(first, last, begin()); // TODO use memcpy?
    }

    mvalue(const mvalue& rhs) :
        vt_(&vtempty[rhs.get_value_type()]) // TODO be careful of nil_type
    {
        // TODO may be better way to do more direct copy
        vt_->raw_insert(*this, 0, rhs.size());
        std::copy(rhs.begin(), rhs.end(), begin()); // TODO use memcpy?
    }

    ~mvalue()
    {
        vt_->dtor(*this);
    }

    mvalue& operator =(const mvalue& rhs)
    {
        vt_->dtor(*this);
        vt_ = &vtempty[rhs.get_value_type()];
        vt_->raw_insert(*this, 0, rhs.size());
        std::copy(rhs.begin(), rhs.end(), begin()); // TODO use memcpy?
        return *this;
    }

    template <typename InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        vt_->dtor(*this);
        vt_ = &vtempty[ftraits<std::iterator_traits<InputIterator>::value_type>::value_type];
        vt_->raw_insert(*this, 0, std::distance(first, last));
        std::copy(first, last, begin()); // TODO use memcpy?
    }
    template <>
    void assign(const_iterator first, const_iterator last)
    {
        vt_->dtor(*this);
        vt_ = &vtempty[first.get_value_type()];
        vt_->raw_insert(*this, 0, last - first);
        std::copy(first, last, begin()); // TODO use memcpy?
    }
    template <>
    void assign(iterator first, iterator last)
    {
        vt_->dtor(*this);
        vt_ = &vtempty[first.get_value_type()];
        vt_->raw_insert(*this, 0, last - first);
        std::copy(first, last, begin()); // TODO use memcpy?
    }

#define MVALUE_ASSIGN(T) \
    void assign(size_type n, T t) \
    { \
        vt_->dtor(*this); \
        vt_ = &vtempty[ftraits<T>::value_type]; \
        vt_->raw_insert(*this, 0, n); \
        std::uninitialized_fill_n(static_cast<T*>(ptr()), n, t); \
    }
    MVALUE_ASSIGN(bool)
    MVALUE_ASSIGN(char)
    MVALUE_ASSIGN(wchar_t)
    MVALUE_ASSIGN(signed char)
    MVALUE_ASSIGN(unsigned char)
    MVALUE_ASSIGN(short)
    MVALUE_ASSIGN(unsigned short)
    MVALUE_ASSIGN(int)
    MVALUE_ASSIGN(unsigned int)
    MVALUE_ASSIGN(long)
    MVALUE_ASSIGN(unsigned long)
    MVALUE_ASSIGN(long long)
    MVALUE_ASSIGN(unsigned long long)
    MVALUE_ASSIGN(float)
    MVALUE_ASSIGN(double)
    MVALUE_ASSIGN(long double)
#undef MVALUE_ASSIGN

public: // iterators

    iterator begin()                                        { return iterator(this); }
    const_iterator begin() const                            { return const_iterator(this); }

    iterator end()                                          { return iterator(this, size()); }
    const_iterator end() const                              { return const_iterator(this, size()); }

    reverse_iterator rbegin()                               { return reverse_iterator(iterator(this, size())); }
    const_reverse_iterator rbegin() const                   { return const_reverse_iterator(const_iterator(this, size())); }

    reverse_iterator rend()                                 { return reverse_iterator(iterator(this)); }
    const_reverse_iterator rend() const                     { return const_reverse_iterator(const_iterator(this)); }

public: // capacity

    size_type size() const                                  { return vt_->size(*this); }
    size_type max_size() const                              { return get_value_type() == nil_type ? 0 : 1000000; } // TODO remove hardcoding

    void resize(size_type n)                                { resize(n, 0); }

#define MVALUE_RESIZE(T) \
    void resize(size_type n, T t) \
    { \
        size_type sz = size(); \
        if (n < sz) \
        { \
            vt_->raw_erase(*this, n, sz - n); \
        } \
        else if (sz < n) \
        { \
            insert(end(), n - sz, t); \
        } \
    }
    MVALUE_RESIZE(bool)
    MVALUE_RESIZE(char)
    MVALUE_RESIZE(wchar_t)
    MVALUE_RESIZE(signed char)
    MVALUE_RESIZE(unsigned char)
    MVALUE_RESIZE(short)
    MVALUE_RESIZE(unsigned short)
    MVALUE_RESIZE(int)
    MVALUE_RESIZE(unsigned int)
    MVALUE_RESIZE(long)
    MVALUE_RESIZE(unsigned long)
    MVALUE_RESIZE(long long)
    MVALUE_RESIZE(unsigned long long)
    MVALUE_RESIZE(float)
    MVALUE_RESIZE(double)
    MVALUE_RESIZE(long double)
#undef MVALUE_RESIZE

    bool empty() const                                      { return size() == 0; }

public: // element access

    reference operator [](size_type n)                      { return reference(this, n); }
    const_reference operator [](size_type n) const          { return const_reference(this, n); }

    reference at(size_type n)                               { if (size() <= n) throw std::out_of_range("oops"); return reference(this, n); }
    const_reference at(size_type n) const                   { if (size() <= n) throw std::out_of_range("oops"); return const_reference(this, n); }

    reference front()                                       { return reference(this); }
    const_reference front() const                           { return const_reference(this); }

    reference back()                                        { return reference(this, size() - 1); }
    const_reference back() const                            { return const_reference(this, size() - 1); }

public: // modifiers

    // iterator vector<T>::insert(iterator position, const T& x)
    // void vector<T>::insert(iterator position, size_type n, const T& x)
#define MVALUE_INSERT(T) \
    iterator insert(const iterator& p, T t)                 { insert(p, size_type(1), t); return p; } \
    void insert(const iterator& p, size_type n, T t) \
    { \
        vt_->raw_insert(*this, p.n_, n); \
        switch (get_value_type()) \
        { \
            case bool_type:         std::uninitialized_fill_n(bool_ptr(p.n_), n, t); break; \
            case char_type:         std::uninitialized_fill_n(char_ptr(p.n_), n, t); break; \
            case wchar_type:        std::uninitialized_fill_n(wchar_ptr(p.n_), n, t); break; \
            case schar_type:        std::uninitialized_fill_n(schar_ptr(p.n_), n, t); break; \
            case uchar_type:        std::uninitialized_fill_n(uchar_ptr(p.n_), n, t); break; \
            case short_type:        std::uninitialized_fill_n(short_ptr(p.n_), n, t); break; \
            case ushort_type:       std::uninitialized_fill_n(ushort_ptr(p.n_), n, t); break; \
            case int_type:          std::uninitialized_fill_n(int_ptr(p.n_), n, t); break; \
            case uint_type:         std::uninitialized_fill_n(uint_ptr(p.n_), n, t); break; \
            case long_type:         std::uninitialized_fill_n(long_ptr(p.n_), n, t); break; \
            case ulong_type:        std::uninitialized_fill_n(ulong_ptr(p.n_), n, t); break; \
            case longlong_type:     std::uninitialized_fill_n(longlong_ptr(p.n_), n, t); break; \
            case ulonglong_type:    std::uninitialized_fill_n(ulonglong_ptr(p.n_), n, t); break; \
            case float_type:        std::uninitialized_fill_n(float_ptr(p.n_), n, t); break; \
            case double_type:       std::uninitialized_fill_n(double_ptr(p.n_), n, t); break; \
            case ldouble_type:      std::uninitialized_fill_n(ldouble_ptr(p.n_), n, t); break; \
            default:                assert(false); \
        } \
    } \
    /* Added int version to force overload resolution */ \
    void insert(const iterator& p, int n, T t)              { insert(p, size_type(n), t); }
    MVALUE_INSERT(bool)
    MVALUE_INSERT(char)
    MVALUE_INSERT(wchar_t)
    MVALUE_INSERT(signed char)
    MVALUE_INSERT(unsigned char)
    MVALUE_INSERT(short)
    MVALUE_INSERT(unsigned short)
    MVALUE_INSERT(int)
    MVALUE_INSERT(unsigned int)
    MVALUE_INSERT(long)
    MVALUE_INSERT(unsigned long)
    MVALUE_INSERT(long long)
    MVALUE_INSERT(unsigned long long)
    MVALUE_INSERT(float)
    MVALUE_INSERT(double)
    MVALUE_INSERT(long double)
#undef MVALUE_INSERT

    // void vector<T>::insert(iterator position, InputIterator first, InputIterator last)
    template <typename InputIterator>
    void insert(iterator p, InputIterator first, InputIterator last)
    {
        vt_->raw_insert(*this, p.n_, std::distance(first, last));
        std::copy(first, last, p); // TODO use memcpy?
    }
    // TODO seems specializations are not needed?

    // iterator vector<T>::erase(iterator position)
    iterator erase(iterator p)
    {
        vt_->raw_erase(*this, p.n_, 1);
        return p;
    }

    // iterator vector<T>::erase(iterator first, iterator last)
    iterator erase(iterator first, iterator last)
    {
        vt_->raw_erase(*this, first.n_, last - first);
        return first;
    }

    // void vector<T>::swap(vector<T>& rhs)
    void swap(mvalue& rhs)
    {
        std::swap(vt_, rhs.vt_);
#if MVALUE_LONGLONG_SUPPORT
        std::swap(
            *reinterpret_cast<long long*>(buf_),
            *reinterpret_cast<long long*>(rhs.buf_));
#else
        std::swap(
            *reinterpret_cast<int*>(buf_),
            *reinterpret_cast<int*>(rhs.buf_));
        std::swap(
            *reinterpret_cast<int*>(buf_ + halfbufsz_),
            *reinterpret_cast<int*>(rhs.buf_ + halfbufsz_));
#endif
    }

    // void vector<T>::clear()
    void clear()                            { vt_->raw_erase(*this, 0, size()); }

public: // operators

    bool operator ==(const mvalue& rhs) const
    {
        size_type sz = size();
        if (rhs.size() != sz) return false;
        switch (uac_type(*this, rhs))
        {
            case int_type:          for (size_type n = 0; n != sz; ++n) if (get_int(n) != rhs.get_int(n)) return false; break;
            case uint_type:         for (size_type n = 0; n != sz; ++n) if (get_uint(n) != rhs.get_uint(n)) return false; break;
            case long_type:         for (size_type n = 0; n != sz; ++n) if (get_long(n) != rhs.get_long(n)) return false; break;
            case ulong_type:        for (size_type n = 0; n != sz; ++n) if (get_ulong(n) != rhs.get_ulong(n)) return false; break;
            case longlong_type:     for (size_type n = 0; n != sz; ++n) if (get_longlong(n) != rhs.get_longlong(n)) return false; break;
            case ulonglong_type:    for (size_type n = 0; n != sz; ++n) if (get_ulonglong(n) != rhs.get_ulonglong(n)) return false; break;
            case float_type:        for (size_type n = 0; n != sz; ++n) if (get_float(n) != rhs.get_float(n)) return false; break;
            case double_type:       for (size_type n = 0; n != sz; ++n) if (get_double(n) != rhs.get_double(n)) return false; break;
            case ldouble_type:      for (size_type n = 0; n != sz; ++n) if (get_ldouble(n) != rhs.get_ldouble(n)) return false; break;
            default:                assert(false);
        }
        return true;
    }
    bool operator !=(const mvalue& rhs) const
    {
        size_type sz = size();
        if (rhs.size() != sz) return true;
        switch (uac_type(*this, rhs))
        {
            case int_type:          for (size_type n = 0; n != sz; ++n) if (get_int(n) != rhs.get_int(n)) return true; break;
            case uint_type:         for (size_type n = 0; n != sz; ++n) if (get_uint(n) != rhs.get_uint(n)) return true; break;
            case long_type:         for (size_type n = 0; n != sz; ++n) if (get_long(n) != rhs.get_long(n)) return true; break;
            case ulong_type:        for (size_type n = 0; n != sz; ++n) if (get_ulong(n) != rhs.get_ulong(n)) return true; break;
            case longlong_type:     for (size_type n = 0; n != sz; ++n) if (get_longlong(n) != rhs.get_longlong(n)) return true; break;
            case ulonglong_type:    for (size_type n = 0; n != sz; ++n) if (get_ulonglong(n) != rhs.get_ulonglong(n)) return true; break;
            case float_type:        for (size_type n = 0; n != sz; ++n) if (get_float(n) != rhs.get_float(n)) return true; break;
            case double_type:       for (size_type n = 0; n != sz; ++n) if (get_double(n) != rhs.get_double(n)) return true; break;
            case ldouble_type:      for (size_type n = 0; n != sz; ++n) if (get_ldouble(n) != rhs.get_ldouble(n)) return true; break;
            default:                assert(false);
        }
        return false;
    }
    bool operator <(const mvalue& rhs) const
    {
        size_type lsz = size();
        size_type rsz = rhs.size();
        size_type sz = std::min(lsz, rsz);
        switch (uac_type(*this, rhs))
        {
            case int_type:          for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_int(n), rhs.get_int(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case uint_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_uint(n), rhs.get_uint(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case long_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_long(n), rhs.get_long(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case ulong_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulong(n), rhs.get_ulong(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case longlong_type:     for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_longlong(n), rhs.get_longlong(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case ulonglong_type:    for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulonglong(n), rhs.get_ulonglong(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case float_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_float(n), rhs.get_float(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case double_type:       for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_double(n), rhs.get_double(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case ldouble_type:      for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ldouble(n), rhs.get_ldouble(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            default:                assert(false);
        }
        return lsz < rsz;
    }
    bool operator <=(const mvalue& rhs) const
    {
        size_type lsz = size();
        size_type rsz = rhs.size();
        size_type sz = std::min(lsz, rsz);
        switch (uac_type(*this, rhs))
        {
            case int_type:          for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_int(n), rhs.get_int(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case uint_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_uint(n), rhs.get_uint(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case long_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_long(n), rhs.get_long(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case ulong_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulong(n), rhs.get_ulong(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case longlong_type:     for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_longlong(n), rhs.get_longlong(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case ulonglong_type:    for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulonglong(n), rhs.get_ulonglong(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case float_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_float(n), rhs.get_float(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case double_type:       for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_double(n), rhs.get_double(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            case ldouble_type:      for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ldouble(n), rhs.get_ldouble(n)); if (cmp < 0) return true; else if (0 < cmp) return false; } break;
            default:                assert(false);
        }
        return lsz <= rsz;
    }
    bool operator >(const mvalue& rhs) const
    {
        size_type lsz = size();
        size_type rsz = rhs.size();
        size_type sz = std::min(lsz, rsz);
        switch (uac_type(*this, rhs))
        {
            case int_type:          for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_int(n), rhs.get_int(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case uint_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_uint(n), rhs.get_uint(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case long_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_long(n), rhs.get_long(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case ulong_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulong(n), rhs.get_ulong(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case longlong_type:     for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_longlong(n), rhs.get_longlong(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case ulonglong_type:    for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulonglong(n), rhs.get_ulonglong(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case float_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_float(n), rhs.get_float(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case double_type:       for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_double(n), rhs.get_double(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case ldouble_type:      for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ldouble(n), rhs.get_ldouble(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            default:                assert(false);
        }
        return lsz > rsz;
    }
    bool operator >=(const mvalue& rhs) const
    {
        size_type lsz = size();
        size_type rsz = rhs.size();
        size_type sz = std::min(lsz, rsz);
        switch (uac_type(*this, rhs))
        {
            case int_type:          for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_int(n), rhs.get_int(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case uint_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_uint(n), rhs.get_uint(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case long_type:         for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_long(n), rhs.get_long(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case ulong_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulong(n), rhs.get_ulong(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case longlong_type:     for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_longlong(n), rhs.get_longlong(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case ulonglong_type:    for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ulonglong(n), rhs.get_ulonglong(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case float_type:        for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_float(n), rhs.get_float(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case double_type:       for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_double(n), rhs.get_double(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            case ldouble_type:      for (size_type n = 0; n != sz; ++n) { int cmp = tcmp(get_ldouble(n), rhs.get_ldouble(n)); if (cmp < 0) return false; else if (0 < cmp) return true; } break;
            default:                assert(false);
        }
        return lsz >= rsz;
    }

#define OPERATOR(OP) \
    mvalue& operator OP##=(const mvalue& rhs) \
    { \
        assert(size() == rhs.size()); \
        size_type sz = size(); \
        switch (uac_type(*this, rhs)) \
        { \
            case int_type:          for (size_type n = 0; n != sz; ++n) set_int(n, get_int(n)               OP rhs.get_int(n));         break; \
            case uint_type:         for (size_type n = 0; n != sz; ++n) set_uint(n, get_uint(n)             OP rhs.get_uint(n));        break; \
            case long_type:         for (size_type n = 0; n != sz; ++n) set_long(n, get_long(n)             OP rhs.get_long(n));        break; \
            case ulong_type:        for (size_type n = 0; n != sz; ++n) set_ulong(n, get_ulong(n)           OP rhs.get_ulong(n));       break; \
            case longlong_type:     for (size_type n = 0; n != sz; ++n) set_longlong(n, get_longlong(n)     OP rhs.get_longlong(n));    break; \
            case ulonglong_type:    for (size_type n = 0; n != sz; ++n) set_ulonglong(n, get_ulonglong(n)   OP rhs.get_ulonglong(n));   break; \
            case float_type:        for (size_type n = 0; n != sz; ++n) set_float(n, get_float(n)           OP rhs.get_float(n));       break; \
            case double_type:       for (size_type n = 0; n != sz; ++n) set_double(n, get_double(n)         OP rhs.get_double(n));      break; \
            case ldouble_type:      for (size_type n = 0; n != sz; ++n) set_ldouble(n, get_ldouble(n)       OP rhs.get_ldouble(n));     break; \
            default:                assert(false); \
        } \
        return *this; \
    }
    OPERATOR(+)
    OPERATOR(-)
    OPERATOR(*)
    OPERATOR(/)
    //OPERATOR(%)
#undef OPERATOR

private:

    template <typename T>
    static int tcmp(T t1, T t2)                                 { if (t1 < t2) return -1; else if (t2 < t1) return 1; else return 0; }

    void* ptr()                                                 { return const_cast<void*>(vt_->ptr(*this, 0)); }

    void* ptr(size_type n)                                      { assert(n < size()); return const_cast<void*>(vt_->ptr(*this, n)); }
    const void* ptr(size_type n) const                          { assert(n < size()); return vt_->ptr(*this, n); }

    // TODO can these be static_cast?
    bool* bool_ptr(size_type n)                                 { assert(get_value_type() == bool_type);        return reinterpret_cast<bool*>(ptr(n)); }
    char* char_ptr(size_type n)                                 { assert(get_value_type() == char_type);        return reinterpret_cast<char*>(ptr(n)); }
    wchar_t* wchar_ptr(size_type n)                             { assert(get_value_type() == wchar_type);       return reinterpret_cast<wchar_t*>(ptr(n)); }
    signed char* schar_ptr(size_type n)                         { assert(get_value_type() == schar_type);       return reinterpret_cast<signed char*>(ptr(n)); }
    unsigned char* uchar_ptr(size_type n)                       { assert(get_value_type() == uchar_type);       return reinterpret_cast<unsigned char*>(ptr(n)); }
    short* short_ptr(size_type n)                               { assert(get_value_type() == short_type);       return reinterpret_cast<short*>(ptr(n)); }
    unsigned short* ushort_ptr(size_type n)                     { assert(get_value_type() == ushort_type);      return reinterpret_cast<unsigned short*>(ptr(n)); }
    int* int_ptr(size_type n)                                   { assert(get_value_type() == int_type);         return reinterpret_cast<int*>(ptr(n)); }
    unsigned int* uint_ptr(size_type n)                         { assert(get_value_type() == uint_type);        return reinterpret_cast<unsigned int*>(ptr(n)); }
    long* long_ptr(size_type n)                                 { assert(get_value_type() == long_type);        return reinterpret_cast<long*>(ptr(n)); }
    unsigned long* ulong_ptr(size_type n)                       { assert(get_value_type() == ulong_type);       return reinterpret_cast<unsigned long*>(ptr(n)); }
    long long* longlong_ptr(size_type n)                        { assert(get_value_type() == longlong_type);    return reinterpret_cast<long long*>(ptr(n)); }
    unsigned long long* ulonglong_ptr(size_type n)              { assert(get_value_type() == ulonglong_type);   return reinterpret_cast<unsigned long long*>(ptr(n)); }
    float* float_ptr(size_type n)                               { assert(get_value_type() == float_type);       return reinterpret_cast<float*>(ptr(n)); }
    double* double_ptr(size_type n)                             { assert(get_value_type() == double_type);      return reinterpret_cast<double*>(ptr(n)); }
    long double* ldouble_ptr(size_type n)                       { assert(get_value_type() == ldouble_type);     return reinterpret_cast<long double*>(ptr(n)); }

    const bool* bool_ptr(size_type n) const                     { assert(get_value_type() == bool_type);        return reinterpret_cast<const bool*>(ptr(n)); }
    const char* char_ptr(size_type n) const                     { assert(get_value_type() == char_type);        return reinterpret_cast<const char*>(ptr(n)); }
    const wchar_t* wchar_ptr(size_type n) const                 { assert(get_value_type() == wchar_type);       return reinterpret_cast<const wchar_t*>(ptr(n)); }
    const signed char* schar_ptr(size_type n) const             { assert(get_value_type() == schar_type);       return reinterpret_cast<const signed char*>(ptr(n)); }
    const unsigned char* uchar_ptr(size_type n) const           { assert(get_value_type() == uchar_type);       return reinterpret_cast<const unsigned char*>(ptr(n)); }
    const short* short_ptr(size_type n) const                   { assert(get_value_type() == short_type);       return reinterpret_cast<const short*>(ptr(n)); }
    const unsigned short* ushort_ptr(size_type n) const         { assert(get_value_type() == ushort_type);      return reinterpret_cast<const unsigned short*>(ptr(n)); }
    const int* int_ptr(size_type n) const                       { assert(get_value_type() == int_type);         return reinterpret_cast<const int*>(ptr(n)); }
    const unsigned int* uint_ptr(size_type n) const             { assert(get_value_type() == uint_type);        return reinterpret_cast<const unsigned int*>(ptr(n)); }
    const long* long_ptr(size_type n) const                     { assert(get_value_type() == long_type);        return reinterpret_cast<const long*>(ptr(n)); }
    const unsigned long* ulong_ptr(size_type n) const           { assert(get_value_type() == ulong_type);       return reinterpret_cast<const unsigned long*>(ptr(n)); }
    const long long* longlong_ptr(size_type n) const            { assert(get_value_type() == longlong_type);    return reinterpret_cast<const long long*>(ptr(n)); }
    const unsigned long long* ulonglong_ptr(size_type n) const  { assert(get_value_type() == ulonglong_type);   return reinterpret_cast<const unsigned long long*>(ptr(n)); }
    const float* float_ptr(size_type n) const                   { assert(get_value_type() == float_type);       return reinterpret_cast<const float*>(ptr(n)); }
    const double* double_ptr(size_type n) const                 { assert(get_value_type() == double_type);      return reinterpret_cast<const double*>(ptr(n)); }
    const long double* ldouble_ptr(size_type n) const           { assert(get_value_type() == ldouble_type);     return reinterpret_cast<const long double*>(ptr(n)); }

    int get_int(size_type n) const                              { return vt_->get_int(*this, n); }
    unsigned int get_uint(size_type n) const                    { return vt_->get_uint(*this, n); }
    long get_long(size_type n) const                            { return vt_->get_long(*this, n); }
    unsigned long get_ulong(size_type n) const                  { return vt_->get_ulong(*this, n); }
    long long get_longlong(size_type n) const                   { return vt_->get_longlong(*this, n); }
    unsigned long long get_ulonglong(size_type n) const         { return vt_->get_ulonglong(*this, n); }
    float get_float(size_type n) const                          { return vt_->get_float(*this, n); }
    double get_double(size_type n) const                        { return vt_->get_double(*this, n); }
    long double get_ldouble(size_type n) const                  { return vt_->get_ldouble(*this, n); }

    void set_int(size_type n, int t)                            { vt_->set_int(*this, n, t); }
    void set_uint(size_type n, unsigned int t)                  { vt_->set_uint(*this, n, t); }
    void set_long(size_type n, long t)                          { vt_->set_long(*this, n, t); }
    void set_ulong(size_type n, unsigned long t)                { vt_->set_ulong(*this, n, t); }
    void set_longlong(size_type n, long long t)                 { vt_->set_longlong(*this, n, t); }
    void set_ulonglong(size_type n, unsigned long long t)       { vt_->set_ulonglong(*this, n, t); }
    void set_float(size_type n, float t)                        { vt_->set_float(*this, n, t); }
    void set_double(size_type n, double t)                      { vt_->set_double(*this, n, t); }
    void set_ldouble(size_type n, long double t)                { vt_->set_ldouble(*this, n, t); }

    void clear_buf()                                            { *reinterpret_cast<long double*>(buf_) = 0; }

    static value_type uac_type(const mvalue& lhs, const mvalue& rhs) { return uac_table[lhs.get_value_type()][rhs.get_value_type()]; }

public:

    static value_type uac_type(mvalue::value_type lhs, mvalue::value_type rhs) { return uac_table[lhs][rhs]; }

private:

    // Fake vtable idiom as in Alexandrescu article in CUJ June 2002.
    struct vtbl; friend vtbl;

    enum { bufsz_ = sizeof(long double), halfbufsz_ = bufsz_/2 };

    static const size_type max_small_size[];

    static const value_type uac_table[16][16];

    vtbl* vt_;
    char buf_[bufsz_];

private:

    struct util;        friend util;        static vtbl vtnil;
    struct emptyx;      friend emptyx;      static vtbl vtempty[];
    struct single;      friend single;      static vtbl vtsingle[];
    struct small;       friend small;       // TODO small vtbl
    struct large;       friend large;       static vtbl vtlarge[];

struct vtbl
{
    // types
    value_type          (*get_value_type)   ();

    // construct/copy/destroy
    void                (*dtor)             (const mvalue& self);

    // capacity
    size_type           (*size)             (const mvalue& self);

    // element access
    const void*         (*ptr)              (const mvalue& self, size_type n);
    int                 (*get_int)          (const mvalue& self, size_type n);
    unsigned int        (*get_uint)         (const mvalue& self, size_type n);
    long                (*get_long)         (const mvalue& self, size_type n);
    unsigned long       (*get_ulong)        (const mvalue& self, size_type n);
    long long           (*get_longlong)     (const mvalue& self, size_type n);
    unsigned long long  (*get_ulonglong)    (const mvalue& self, size_type n);
    float               (*get_float)        (const mvalue& self, size_type n);
    double              (*get_double)       (const mvalue& self, size_type n);
    long double         (*get_ldouble)      (const mvalue& self, size_type n);
    void                (*set_int)          (mvalue& self, size_type n, int t);
    void                (*set_uint)         (mvalue& self, size_type n, unsigned int t);
    void                (*set_long)         (mvalue& self, size_type n, long t);
    void                (*set_ulong)        (mvalue& self, size_type n, unsigned long t);
    void                (*set_longlong)     (mvalue& self, size_type n, long long t);
    void                (*set_ulonglong)    (mvalue& self, size_type n, unsigned long long t);
    void                (*set_float)        (mvalue& self, size_type n, float t);
    void                (*set_double)       (mvalue& self, size_type n, double t);
    void                (*set_ldouble)      (mvalue& self, size_type n, long double t);

    // modifiers
    void                (*raw_insert)       (mvalue& self, size_type p, size_type n);
    void                (*raw_erase)        (mvalue& self, size_type p, size_type n);
};

private:

struct proxy_base
{
protected:

    proxy_base() : pval_(0), n_(0) {}
    proxy_base(mvalue* pval) : pval_(pval), n_(0) {}
    proxy_base(mvalue* pval, size_type n) : pval_(pval), n_(n) {}
    // default copy ctor OK
    // default copy assignment OK

protected:

    mvalue* pval_;
    mvalue::size_type n_;
};

public:

class const_reference : public proxy_base
{
    friend mvalue;                      friend const_iterator;

public:

    mvalue::value_type get_value_type() const   { return pval_->get_value_type(); }

public:

    operator bool() const               { return pval_->get_int(n_) != 0; }
    operator char() const               { return pval_->get_int(n_); }
    operator wchar_t() const            { return pval_->get_int(n_); }
    operator signed char() const        { return pval_->get_int(n_); }
    operator unsigned char() const      { return pval_->get_int(n_); }
    operator short() const              { return pval_->get_int(n_); }
    operator unsigned short() const     { return pval_->get_int(n_); }
    operator int() const                { return pval_->get_int(n_); }
    operator unsigned int() const       { return pval_->get_uint(n_); }
    operator long() const               { return pval_->get_long(n_); }
    operator unsigned long() const      { return pval_->get_ulong(n_); }
#if MVALUE_LONGLONG_SUPPORT
    operator long long() const          { return pval_->get_longlong(n_); }
    operator unsigned long long() const { return pval_->get_ulonglong(n_); }
#endif
    operator float() const              { return pval_->get_float(n_); }
    operator double() const             { return pval_->get_double(n_); }
    operator long double() const        { return pval_->get_ldouble(n_); }

    // TODO does this work?
//  const_iterator operator &()         { return const_iterator(*this); }

private:

    const_reference(const mvalue* pval) : proxy_base(const_cast<mvalue*>(pval)) {}
    const_reference(const mvalue* pval, size_type n) : proxy_base(const_cast<mvalue*>(pval), n) {}
    const_reference(const proxy_base& rhs) : proxy_base(rhs) {}
};

class reference : public const_reference
{
    friend mvalue;                              friend iterator;

public:

    reference& operator =(bool t)               { pval_->set_int(n_, t); return *this; }
    reference& operator =(char t)               { pval_->set_int(n_, t); return *this; }
    reference& operator =(wchar_t t)            { pval_->set_int(n_, t); return *this; }
    reference& operator =(signed char t)        { pval_->set_int(n_, t); return *this; }
    reference& operator =(unsigned char t)      { pval_->set_int(n_, t); return *this; }
    reference& operator =(short t)              { pval_->set_int(n_, t); return *this; }
    reference& operator =(unsigned short t)     { pval_->set_int(n_, t); return *this; }
    reference& operator =(int t)                { pval_->set_int(n_, t); return *this; }
    reference& operator =(unsigned int t)       { pval_->set_uint(n_, t); return *this; }
    reference& operator =(long t)               { pval_->set_long(n_, t); return *this; }
    reference& operator =(unsigned long t)      { pval_->set_ulong(n_, t); return *this; }
#if MVALUE_LONGLONG_SUPPORT
    reference& operator =(long long t)          { pval_->set_longlong(n_, t); return *this; }
    reference& operator =(unsigned long long t) { pval_->set_ulonglong(n_, t); return *this; }
#endif
    reference& operator =(float t)              { pval_->set_float(n_, t); return *this; }
    reference& operator =(double t)             { pval_->set_double(n_, t); return *this; }
    reference& operator =(long double t)        { pval_->set_ldouble(n_, t); return *this; }

    // TODO need full suite of assignment operators, +=, etc
    reference& operator =(const const_reference& rhs)
    {
        *this = static_cast<double>(rhs); // TODO more dynamic casting
        return *this;
    }

    // TODO apparently the above doesn't act as a copy assignment operator
    reference& operator =(const reference& rhs)
    {
        *this = static_cast<double>(rhs); // TODO more dynamic casting
        return *this;
    }

    // TODO does this work?
//  iterator operator &()                       { return iterator(*this); }

private:

    reference(mvalue* pval) : const_reference(pval) {}
    reference(mvalue* pval, size_type n) : const_reference(pval, n) {}
    reference(const proxy_base& rhs) : const_reference(rhs) {}
};

class const_iterator : public proxy_base
{
    friend mvalue;                              friend const_reference;

public:

    typedef void                                value_type;
    typedef mvalue::difference_type             difference_type;
    typedef mvalue::pointer                     pointer;
    typedef mvalue::const_reference             reference;
    typedef std::random_access_iterator_tag     iterator_category;

    mvalue::value_type get_value_type() const   { return pval_->get_value_type(); }

public: // http://www.sgi.com/tech/stl/Assignable.html

    // default copy ctor OK
    // default copy assignment OK

    void swap(const_iterator& rhs)
    {
        std::swap(pval_, rhs.pval_);
        std::swap(n_, rhs.n_);
    }

public: // http://www.sgi.com/tech/stl/EqualityComparable.html

    bool operator ==(const const_iterator& rhs) const
    {
        return pval_ == rhs.pval_ && n_ == rhs.n_;
    }

    bool operator !=(const const_iterator& rhs) const
    {
        return pval_ != rhs.pval_ || n_ != rhs.n_;
    }

public: // http://www.sgi.com/tech/stl/DefaultConstructible.html

    const_iterator() {}

public: // http://www.sgi.com/tech/stl/trivial.html

    const_reference operator *() const
    {
        return const_reference(*this);
    }

public: // http://www.sgi.com/tech/stl/InputIterator.html

    const_iterator& operator ++()
    {
        ++n_;
        return *this;
    }

    const_iterator operator ++(int)
    {
        const_iterator i(*this); // nrvo
        ++n_;
        return i;
    }

public: // http://www.sgi.com/tech/stl/OutputIterator.html

    // already covered

public: // http://www.sgi.com/tech/stl/ForwardIterator.html

    // already covered

public: // http://www.sgi.com/tech/stl/BidirectionalIterator.html

    const_iterator& operator --()
    {
        --n_;
        return *this;
    }

    const_iterator operator --(int)
    {
        const_iterator i(*this); // nrvo
        --n_;
        return i;
    }

public: // http://www.sgi.com/tech/stl/LessThanComparable.html

    bool operator <(const const_iterator& rhs) const
    {
        return pval_ == rhs.pval_ && n_ < rhs.n_;
    }

    bool operator <=(const const_iterator& rhs) const
    {
        return pval_ == rhs.pval_ && n_ <= rhs.n_;
    }

    bool operator >(const const_iterator& rhs) const
    {
        return pval_ == rhs.pval_ && n_ > rhs.n_;
    }

    bool operator >=(const const_iterator& rhs) const
    {
        return pval_ == rhs.pval_ && n_ >= rhs.n_;
    }

public: // http://www.sgi.com/tech/stl/RandomAccessIterator.html

    const_iterator& operator +=(difference_type n)
    {
        n_ += n;
        return *this;
    }

    const_iterator operator +(difference_type n) const
    {
        return const_iterator(*this) += n;
    }

    const_iterator& operator -=(difference_type n)
    {
        n_ -= n;
        return *this;
    }

    const_iterator operator -(difference_type n) const
    {
        return const_iterator(*this) -= n;
    }

    difference_type operator -(const const_iterator& rhs) const
    {
        return n_ - rhs.n_;
    }

    const_reference operator [](size_type n) const
    {
        return const_reference(pval_, n_ + n);
    }

private:

    const_iterator(const mvalue* pval) : proxy_base(const_cast<mvalue*>(pval)) {}
    const_iterator(const mvalue* pval, size_type n) : proxy_base(const_cast<mvalue*>(pval), n) {}
};

class iterator : public const_iterator
{
    friend mvalue;

public:

    typedef void                                value_type;
    typedef mvalue::difference_type             difference_type;
    typedef mvalue::pointer                     pointer;
    typedef mvalue::reference                   reference;
    typedef std::random_access_iterator_tag     iterator_category;

public:

    iterator() {}

public: // http://www.sgi.com/tech/stl/trivial.html

    reference operator *() const
    {
        return reference(*this);
    }

public: // http://www.sgi.com/tech/stl/InputIterator.html

    iterator& operator ++()
    {
        ++n_;
        return *this;
    }

    iterator operator ++(int)
    {
        iterator i(*this); // nrvo
        ++n_;
        return i;
    }

public: // http://www.sgi.com/tech/stl/BidirectionalIterator.html

    iterator& operator --()
    {
        --n_;
        return *this;
    }

    iterator operator --(int)
    {
        iterator i(*this); // nrvo
        --n_;
        return i;
    }

public: // http://www.sgi.com/tech/stl/RandomAccessIterator.html

    iterator& operator +=(difference_type n)
    {
        n_ += n;
        return *this;
    }

    iterator operator +(difference_type n) const
    {
        return iterator(*this) += n;
    }

    iterator& operator -=(difference_type n)
    {
        n_ -= n;
        return *this;
    }

    iterator operator -(difference_type n) const
    {
        return iterator(*this) -= n;
    }

    difference_type operator -(const iterator& rhs) const
    {
        return n_ - rhs.n_;
    }

    reference operator [](size_type n) const
    {
        return reference(const_cast<mvalue*>(pval_), n_ + n);
    }

public:

    // TESTING this, seems OK, need to make pval() func call voidpointer() func
    // need more overloads in this class
    // need const overloads in const_iterator
//  operator double*() const { return reinterpret_cast<double*>(const_cast<mvalue*>(pval_)->buf_); }

private:

    iterator(mvalue* pval) : const_iterator(pval) {}
    iterator(mvalue* pval, size_type n) : const_iterator(pval, n) {}
};

}; // mvalue

// TODO need == < != > >= <= for two mvalue
// TODO need swap for two mvalue
// TODO need op+ etc. (with new uac type) for two mvalue

// TODO verify this syntax
namespace std
{
    inline void swap(mvalue& lhs, mvalue& rhs) { lhs.swap(rhs); }
}

inline mvalue::const_iterator operator +(mvalue::difference_type n, const mvalue::const_iterator& i)
{
    return i + n;
}

inline mvalue::iterator operator +(mvalue::difference_type n, const mvalue::iterator& i)
{
    return i + n;
}

#define OPERATOR(OP) \
    inline bool operator OP(const mvalue::const_reference& lhs, const mvalue::const_reference& rhs) \
    { \
        switch (mvalue::uac_type(lhs.get_value_type(), rhs.get_value_type())) \
        { \
            case mvalue::int_type:          return static_cast<int>(lhs)                    OP static_cast<int>(rhs); \
            case mvalue::uint_type:         return static_cast<unsigned int>(lhs)           OP static_cast<unsigned int>(rhs); \
            case mvalue::long_type:         return static_cast<long>(lhs)                   OP static_cast<long>(rhs); \
            case mvalue::ulong_type:        return static_cast<unsigned long>(lhs)          OP static_cast<unsigned long>(rhs); \
            case mvalue::longlong_type:     return static_cast<long long>(lhs)              OP static_cast<long long>(rhs); \
            case mvalue::ulonglong_type:    return static_cast<unsigned long long>(lhs)     OP static_cast<unsigned long long>(rhs); \
            case mvalue::float_type:        return static_cast<float>(lhs)                  OP static_cast<float>(rhs); \
            case mvalue::double_type:       return static_cast<double>(lhs)                 OP static_cast<double>(rhs); \
            case mvalue::ldouble_type:      return static_cast<long double>(lhs)            OP static_cast<long double>(rhs); \
            default:                        assert(false); return false; \
        } \
    }
OPERATOR(==)
OPERATOR(!=)
OPERATOR(<)
OPERATOR(<=)
OPERATOR(>)
OPERATOR(>=)
#undef OPERATOR

#define OPERATOR(OP, T) \
    inline bool operator OP(const mvalue::const_reference& ref, T t) \
    { \
        mvalue v(t); /* TODO use traits */ \
        switch (mvalue::uac_type(ref.get_value_type(), v.get_value_type())) \
        { \
            case mvalue::int_type:          return static_cast<int>(ref) OP t; \
            case mvalue::uint_type:         return static_cast<unsigned int>(ref) OP t; \
            case mvalue::long_type:         return static_cast<long>(ref) OP t; \
            case mvalue::ulong_type:        return static_cast<unsigned long>(ref) OP t; \
            case mvalue::longlong_type:     return static_cast<long long>(ref) OP t; \
            case mvalue::ulonglong_type:    return static_cast<unsigned long long>(ref) OP t; \
            case mvalue::float_type:        return static_cast<float>(ref) OP t; \
            case mvalue::double_type:       return static_cast<double>(ref) OP t; \
            case mvalue::ldouble_type:      return static_cast<long double>(ref) OP t; \
            default:                        assert(false); return false; \
        } \
    }\
    inline bool operator OP(T t, const mvalue::const_reference& ref) \
    { \
        mvalue v(t); /* TODO use traits */ \
        switch (mvalue::uac_type(ref.get_value_type(), v.get_value_type())) \
        { \
            case mvalue::int_type:          return static_cast<int>(ref) OP t; \
            case mvalue::uint_type:         return static_cast<unsigned int>(ref) OP t; \
            case mvalue::long_type:         return static_cast<long>(ref) OP t; \
            case mvalue::ulong_type:        return static_cast<unsigned long>(ref) OP t; \
            case mvalue::longlong_type:     return static_cast<long long>(ref) OP t; \
            case mvalue::ulonglong_type:    return static_cast<unsigned long long>(ref) OP t; \
            case mvalue::float_type:        return static_cast<float>(ref) OP t; \
            case mvalue::double_type:       return static_cast<double>(ref) OP t; \
            case mvalue::ldouble_type:      return static_cast<long double>(ref) OP t; \
            default:                        assert(false); return false; \
        } \
    }
#define OPERATORS(T) \
    OPERATOR(==, T) \
    OPERATOR(!=, T) \
    OPERATOR(<, T) \
    OPERATOR(<=, T) \
    OPERATOR(>, T) \
    OPERATOR(>=, T)
OPERATORS(bool)
OPERATORS(char)
OPERATORS(wchar_t)
OPERATORS(signed char)
OPERATORS(unsigned char)
OPERATORS(short)
OPERATORS(unsigned short)
OPERATORS(int)
OPERATORS(unsigned int)
OPERATORS(long)
OPERATORS(unsigned long)
OPERATORS(long long)
OPERATORS(unsigned long long)
OPERATORS(float)
OPERATORS(double)
OPERATORS(long double)
#undef OPERATORS
#undef OPERATOR

#endif
