// mvalue
// Copyright (C) 2007 Marc Lepage

#include "mvalue.h"

////////////////////////////////////////////////////////////////////////////////

#define MVALUE_UAC(T) \
{ \
    mvalue::ftraits<mvalue::uac<T, bool>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, char>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, wchar_t>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, signed char>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, unsigned char>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, short>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, unsigned short>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, int>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, unsigned int>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, long>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, unsigned long>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, long long>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, unsigned long>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, float>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, double>::result_type>::value_type, \
    mvalue::ftraits<mvalue::uac<T, long double>::result_type>::value_type \
}

const mvalue::value_type mvalue::uac_table[16][16] =
{
    MVALUE_UAC(bool),
    MVALUE_UAC(char),
    MVALUE_UAC(wchar_t),
    MVALUE_UAC(signed char),
    MVALUE_UAC(unsigned char),
    MVALUE_UAC(short),
    MVALUE_UAC(unsigned short),
    MVALUE_UAC(int),
    MVALUE_UAC(unsigned int),
    MVALUE_UAC(long),
    MVALUE_UAC(unsigned long),
    MVALUE_UAC(long long),
    MVALUE_UAC(unsigned long long),
    MVALUE_UAC(float),
    MVALUE_UAC(double),
    MVALUE_UAC(long double)
};

#undef MVALUE_UAC

////////////////////////////////////////////////////////////////////////////////

const mvalue::size_type mvalue::max_small_size[] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

////////////////////////////////////////////////////////////////////////////////

struct mvalue::util
{

template <typename T, T V>
static T literal()
{
    return V;
}

template <typename T, T V>
static T literal(const mvalue& self)
{
    return V;
}

template <typename T, T V>
static T literal(const mvalue& self, size_type n)
{
    return V;
}

static void noop(const mvalue& self)
{
}

static void noop(mvalue& self, size_type n)
{
}

template <typename T1, typename T2, typename T3>
static void noop(T1 t1, T2 t2, T3 t3)
{
}

};

////////////////////////////////////////////////////////////////////////////////

struct mvalue::emptyx
{

template <typename T>
static const void* ptr(const mvalue& self, size_type n)
{
    assert(false);
    return 0;
}

template <typename T>
static void raw_insert(mvalue& self, size_type p, size_type n)
{
    // new_size is n

    assert(p == 0);

    if (n == 0)
    {
    }
    else if (n == 1)
    {
        self.vt_ = &vtsingle[ftraits<T>::value_type];
    }
//  else if (n <= max_small_size[])
//  {
//  }
    else
    {
        self.vt_ = &vtlarge[ftraits<T>::value_type];
        self.clear_buf();
        self.vt_->raw_insert(self, p, n);
    }
}

template <typename T>
static void raw_erase(mvalue& self, size_type p, size_type n)
{
    assert(p == 0 && n == 0);
}


template <typename T>
static mvalue::vtbl make_vtbl()
{
    mvalue::vtbl vt;
    vt.get_value_type       = util::literal<value_type, ftraits<T>::value_type>;
    vt.dtor                 = util::noop;
    vt.size                 = util::literal<size_type, 0>;
    vt.ptr                  = emptyx::ptr<T>;
    vt.get_int              = util::literal<int, 0>;
    vt.get_uint             = util::literal<unsigned int, 0>;
    vt.get_long             = util::literal<long, 0>;
    vt.get_ulong            = util::literal<unsigned long, 0>;
    vt.get_longlong         = util::literal<long long, 0>;
    vt.get_ulonglong        = util::literal<unsigned long long, 0>;
    vt.get_float            = util::literal<float, 0>;
    vt.get_double           = util::literal<double, 0>;
    vt.get_ldouble          = util::literal<long double, 0>;
    vt.set_int              = util::noop<mvalue&, size_type, int>;
    vt.set_uint             = util::noop<mvalue&, size_type, unsigned int>;
    vt.set_long             = util::noop<mvalue&, size_type, long>;
    vt.set_ulong            = util::noop<mvalue&, size_type, unsigned long>;
    vt.set_longlong         = util::noop<mvalue&, size_type, long long>;
    vt.set_ulonglong        = util::noop<mvalue&, size_type, unsigned long long>;
    vt.set_float            = util::noop<mvalue&, size_type, float>;
    vt.set_double           = util::noop<mvalue&, size_type, double>;
    vt.set_ldouble          = util::noop<mvalue&, size_type, long double>;
    vt.raw_insert           = emptyx::raw_insert<T>;
    vt.raw_erase            = emptyx::raw_erase<T>;
    return vt;
}

};

////////////////////////////////////////////////////////////////////////////////

struct mvalue::single
{

template <typename T>
static T*& data_ref(mvalue& self)
{
    return reinterpret_cast<T*&>(self.buf_);
}

static const void* ptr(const mvalue& self, size_type n)
{
    assert(n == 0);
    return self.buf_;
}

template <typename T, typename TR>
static TR get_elem(const mvalue& self, size_type n)
{
    return *reinterpret_cast<const T*>(self.buf_);
}

template <typename T, typename TR>
static void set_elem(mvalue& self, size_type n, TR t)
{
    *reinterpret_cast<T*>(self.buf_) = t;
}

template <typename T>
static void raw_insert(mvalue& self, size_type p, size_type n)
{
    assert(p <= 1);

    // new_size is n + 1

    if (0 < n)
    {
        // preserve old value
        T t = *data_ref<T>(self);
        // delegate to empty::raw_insert
        emptyx::raw_insert<T>(self, p, n + 1);
        // copy to front or back
        *reinterpret_cast<T*>(self.ptr(p != 0 ? 0 : n)) = t;
    }
}

template <typename T>
static void raw_erase(mvalue& self, size_type p, size_type n)
{
    assert(p == 0 && n <= 1);

    if (n == 1)
    {
        self.vt_ = &vtempty[ftraits<T>::value_type];
    }
}

template <typename T>
static mvalue::vtbl make_vtbl()
{
    mvalue::vtbl vt;
    vt.get_value_type       = util::literal<value_type, ftraits<T>::value_type>;
    vt.dtor                 = util::noop;
    vt.size                 = util::literal<size_type, 1>;
    vt.ptr                  = single::ptr;
    vt.get_int              = single::get_elem<T, int>;
    vt.get_uint             = single::get_elem<T, unsigned int>;
    vt.get_long             = single::get_elem<T, long>;
    vt.get_ulong            = single::get_elem<T, unsigned long>;
    vt.get_longlong         = single::get_elem<T, long long>;
    vt.get_ulonglong        = single::get_elem<T, unsigned long long>;
    vt.get_float            = single::get_elem<T, float>;
    vt.get_double           = single::get_elem<T, double>;
    vt.get_ldouble          = single::get_elem<T, long double>;
    vt.set_int              = single::set_elem<T, int>;
    vt.set_uint             = single::set_elem<T, unsigned int>;
    vt.set_long             = single::set_elem<T, long>;
    vt.set_ulong            = single::set_elem<T, unsigned long>;
    vt.set_longlong         = single::set_elem<T, long long>;
    vt.set_ulonglong        = single::set_elem<T, unsigned long long>;
    vt.set_float            = single::set_elem<T, float>;
    vt.set_double           = single::set_elem<T, double>;
    vt.set_ldouble          = single::set_elem<T, long double>;
    vt.raw_insert           = single::raw_insert<T>;
    vt.raw_erase            = single::raw_erase<T>;
    return vt;
}

};

////////////////////////////////////////////////////////////////////////////////

struct mvalue::small
{

static size_type size(const mvalue& self)
{
    return self.buf_[bufsz_ - 1];
}

template <typename T>
static int get_int(const mvalue& self, size_type n)
{
    return *(reinterpret_cast<const T*>(self.buf_) + n);
}

template <typename T>
static mvalue::vtbl make_vtbl()
{
    mvalue::vtbl vt;
#if 0
    vt.get_value_type       = util::literal<value_type, mvalue_traits<T>::v>;
    vt.dtor                 = util::noop;
    vt.size                 = util::literal<size_type, 0>;
    vt.get_int              = util::literal<int, 0>;
    vt.set_int              = util::noop;
    vt.get_double           = util::literal<double, 0>;
    vt.set_double           = util::noop;
#endif
    return vt;
}

};

////////////////////////////////////////////////////////////////////////////////

struct mvalue::large
{

template <typename T>
inline
static T*& data_ref(mvalue& self)
{
    return *reinterpret_cast<T**>(self.buf_);
}

inline
static size_type& size_ref(mvalue& self)
{
    return *reinterpret_cast<size_type*>(self.buf_ + halfbufsz_);
}

static size_type size(const mvalue& self)
{
    return size_ref(const_cast<mvalue&>(self));
}

template <typename T>
static void dtor(const mvalue& self)
{
    delete[] data_ref<T>(const_cast<mvalue&>(self));
}

template <typename T>
static const void* ptr(const mvalue& self, size_type n)
{
    return *reinterpret_cast<const T* const *>(self.buf_) + n;
}

template <typename T, typename TR>
static TR get_elem(const mvalue& self, size_type n)
{
    return *(*reinterpret_cast<const T* const *>(self.buf_) + n);
}

template <typename T, typename TR>
static void set_elem(mvalue& self, size_type n, TR t)
{
    *(*reinterpret_cast<T**>(self.buf_) + n) = t;
}

template <typename T>
static void raw_insert(mvalue& self, size_type p, size_type n)
{
    if (0 < n)
    {
        T*& data = data_ref<T>(self);
        size_type& size = size_ref(self);
        size_type new_size = size + n;
        T* new_data = new T[new_size];
        memcpy(new_data, data, p * sizeof(T));
        memcpy(new_data + p + n, data + p, (size - p) * sizeof(T));
        delete[] data;
        data = new_data;
        size = new_size;
    }
}

template <typename T>
static void raw_erase(mvalue& self, size_type p, size_type n)
{
    T*& data = data_ref<T>(self);
    size_type& size = size_ref(self);
    size_type new_size = size - n;

    if (new_size == 0)
    {
        delete[] data;
        self.vt_ = &vtempty[ftraits<T>::value_type];
    }
    else if (new_size == 1)
    {
        T t = *(data + (p != 0 ? 0 : n));
        delete[] data;
        self.vt_ = &vtsingle[ftraits<T>::value_type];
        *reinterpret_cast<T*>(self.buf_) = t;
    }
//  else if (new_size <= max_small_size[self.get_value_type()])
//  {
//      delete[] data;
//      // TODO copy elements, change vtbl, etc.
//  }
    else if (new_size < size) // TODO maybe put this check above, or omit
    {
        T* new_data = new T[new_size];
        memcpy(new_data, data, p * sizeof(T));
        memcpy(new_data + p, data + p + n, (new_size - p) * sizeof(T));
        delete[] data;
        data = new_data;
        size = new_size;
    }
}

template <typename T>
static mvalue::vtbl make_vtbl()
{
    mvalue::vtbl vt;
    vt.get_value_type       = util::literal<value_type, ftraits<T>::value_type>;
    vt.dtor                 = large::dtor<T>;
    vt.size                 = large::size;
    vt.ptr                  = large::ptr<T>;
    vt.get_int              = large::get_elem<T, int>;
    vt.get_uint             = large::get_elem<T, unsigned int>;
    vt.get_long             = large::get_elem<T, long>;
    vt.get_ulong            = large::get_elem<T, unsigned long>;
    vt.get_longlong         = large::get_elem<T, long long>;
    vt.get_ulonglong        = large::get_elem<T, unsigned long long>;
    vt.get_float            = large::get_elem<T, float>;
    vt.get_double           = large::get_elem<T, double>;
    vt.get_ldouble          = large::get_elem<T, long double>;
    vt.set_int              = large::set_elem<T, int>;
    vt.set_uint             = large::set_elem<T, unsigned int>;
    vt.set_long             = large::set_elem<T, long>;
    vt.set_ulong            = large::set_elem<T, unsigned long>;
    vt.set_longlong         = large::set_elem<T, long long>;
    vt.set_ulonglong        = large::set_elem<T, unsigned long long>;
    vt.set_float            = large::set_elem<T, float>;
    vt.set_double           = large::set_elem<T, double>;
    vt.set_ldouble          = large::set_elem<T, long double>;
    vt.raw_insert           = large::raw_insert<T>;
    vt.raw_erase            = large::raw_erase<T>;
    return vt;
}

};

////////////////////////////////////////////////////////////////////////////////

mvalue::vtbl mvalue::vtempty[] =
{
    emptyx::make_vtbl<bool>(),
    emptyx::make_vtbl<char>(),
    emptyx::make_vtbl<wchar_t>(),
    emptyx::make_vtbl<signed char>(),
    emptyx::make_vtbl<unsigned char>(),
    emptyx::make_vtbl<short>(),
    emptyx::make_vtbl<unsigned short>(),
    emptyx::make_vtbl<int>(),
    emptyx::make_vtbl<unsigned int>(),
    emptyx::make_vtbl<long>(),
    emptyx::make_vtbl<unsigned long>(),
    emptyx::make_vtbl<long long>(),
    emptyx::make_vtbl<unsigned long long>(),
    emptyx::make_vtbl<float>(),
    emptyx::make_vtbl<double>(),
    emptyx::make_vtbl<long double>(),
};

////////////////////////////////////////////////////////////////////////////////

mvalue::vtbl mvalue::vtsingle[] =
{
    single::make_vtbl<bool>(),
    single::make_vtbl<char>(),
    single::make_vtbl<wchar_t>(),
    single::make_vtbl<signed char>(),
    single::make_vtbl<unsigned char>(),
    single::make_vtbl<short>(),
    single::make_vtbl<unsigned short>(),
    single::make_vtbl<int>(),
    single::make_vtbl<unsigned int>(),
    single::make_vtbl<long>(),
    single::make_vtbl<unsigned long>(),
    single::make_vtbl<long long>(),
    single::make_vtbl<unsigned long long>(),
    single::make_vtbl<float>(),
    single::make_vtbl<double>(),
    single::make_vtbl<long double>(),
};

////////////////////////////////////////////////////////////////////////////////

mvalue::vtbl mvalue::vtlarge[] =
{
    large::make_vtbl<bool>(),
    large::make_vtbl<char>(),
    large::make_vtbl<wchar_t>(),
    large::make_vtbl<signed char>(),
    large::make_vtbl<unsigned char>(),
    large::make_vtbl<short>(),
    large::make_vtbl<unsigned short>(),
    large::make_vtbl<int>(),
    large::make_vtbl<unsigned int>(),
    large::make_vtbl<long>(),
    large::make_vtbl<unsigned long>(),
    large::make_vtbl<long long>(),
    large::make_vtbl<unsigned long long>(),
    large::make_vtbl<float>(),
    large::make_vtbl<double>(),
    large::make_vtbl<long double>(),
};

////////////////////////////////////////////////////////////////////////////////

mvalue::vtbl mvalue::vtnil =
{
    util::literal<value_type, nil_type>,                    // get_value_type
    util::noop,                                             // dtor
    util::literal<size_type, 0>,                            // size
    util::literal<const void*, 0>,                          // ptr
    util::literal<int, 0>,                                  // get_int
    util::literal<unsigned int, 0>,                         // get_uint
    util::literal<long, 0>,                                 // get_long
    util::literal<unsigned long, 0>,                        // get_ulong
    util::literal<long long, 0>,                            // get_longlong
    util::literal<unsigned long long, 0>,                   // get_ulonglong
    util::literal<float, 0>,                                // get_float
    util::literal<double, 0>,                               // get_double
    util::literal<long double, 0>,                          // get_ldouble
    util::noop,                                             // set_int
    util::noop,                                             // set_uint
    util::noop,                                             // set_long
    util::noop,                                             // set_ulong
    util::noop,                                             // set_longlong
    util::noop,                                             // set_ulonglong
    util::noop,                                             // set_float
    util::noop,                                             // set_double
    util::noop,                                             // set_ldouble
    util::noop,                                             // raw_insert
    util::noop,                                             // raw_erase
};
