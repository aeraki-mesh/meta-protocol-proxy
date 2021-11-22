#ifndef __JCE_TYPE_H__
#define __JCE_TYPE_H__

#include <netinet/in.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <limits.h>

namespace taf
{
/////////////////////////////////////////////////////////////////////////////////////
typedef bool    Bool;
typedef char    Char;
typedef short   Short;
typedef float   Float;
typedef double  Double;
typedef int     Int32;

typedef unsigned char   UInt8;
typedef unsigned short  UInt16;
typedef unsigned int    UInt32;

#if __WORDSIZE == 64
typedef long    Int64;
#else
typedef long long   Int64;
#endif

#ifndef JCE_MAX_STRING_LENGTH
#define JCE_MAX_STRING_LENGTH   (100 * 1024 * 1024)
#endif
/*
#define jce__bswap_constant_32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |           \
    (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#define jce__bswap_constant_64(x) \
    ((((x) & 0xff00000000000000ull) >> 56)                   \
    | (((x) & 0x00ff000000000000ull) >> 40)                     \
    | (((x) & 0x0000ff0000000000ull) >> 24)                     \
    | (((x) & 0x000000ff00000000ull) >> 8)                      \
    | (((x) & 0x00000000ff000000ull) << 8)                      \
    | (((x) & 0x0000000000ff0000ull) << 24)                     \
    | (((x) & 0x000000000000ff00ull) << 40)                     \
    | (((x) & 0x00000000000000ffull) << 56))
*/
#ifdef __APPLE__
#   ifndef __LITTLE_ENDIAN
#       define __LITTLE_ENDIAN 1234
#   endif
#   ifndef __BIG_ENDIAN
#       define __BIG_ENDIAN    4321
#   endif
#   ifndef __BYTE_ORDER
#       define __BYTE_ORDER __LITTLE_ENDIAN
#   endif
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#   define jce_ntohll(x)    (x)
#   define jce_htonll(x)    (x)
#   define jce_ntohf(x)     (x)
#   define jce_htonf(x)     (x)
#   define jce_ntohd(x)     (x)
#   define jce_htond(x)     (x)
#else
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define jce_ntohll(x)    jce_htonll(x)
//#     define jce_htonll(x)    jce__bswap_constant_64(x)
namespace jce
{
        union bswap_helper
        {
                Int64   i64;
                Int32   i32[2];
        };
}
inline Int64 jce_htonll(Int64 x)
{
        jce::bswap_helper h;
        h.i64 = x;
        Int32 tmp = htonl(h.i32[1]);
        h.i32[1] = htonl(h.i32[0]);
        h.i32[0] = tmp;
        return h.i64;
}
inline Float jce_ntohf(Float x)
{
    union {
        Float f;
        Int32 i32;
    } helper;

    helper.f = x;
    helper.i32 = htonl( helper.i32 );

    return helper.f;
}
#       define jce_htonf(x)     jce_ntohf(x)
inline Double jce_ntohd(Double x)
{
    union {
        Double d;
        Int64 i64;
    } helper;

    helper.d = x;
    helper.i64 = jce_htonll( helper.i64 );

    return helper.d;
}
#       define jce_htond(x)     jce_ntohd(x)
#   endif
#endif

//type2name
template<typename T> struct JceClass    { static std::string name() { return T::className(); } };
template<> struct JceClass<taf::Bool>   { static std::string name() { return "bool"; } };
template<> struct JceClass<taf::Char>   { static std::string name() { return "char"; } };
template<> struct JceClass<taf::Short>  { static std::string name() { return "short"; } };
template<> struct JceClass<taf::Float>  { static std::string name() { return "float"; } };
template<> struct JceClass<taf::Double> { static std::string name() { return "double"; } };
template<> struct JceClass<taf::Int32>  { static std::string name() { return "int32"; } };
template<> struct JceClass<taf::Int64>  { static std::string name() { return "int64"; } };
template<> struct JceClass<taf::UInt8>  { static std::string name() { return "short"; } };
template<> struct JceClass<taf::UInt16> { static std::string name() { return "int32"; } };
template<> struct JceClass<taf::UInt32> { static std::string name() { return "int64"; } };
template<> struct JceClass<std::string> { static std::string name() { return "string"; } };
template<typename T> struct JceClass<std::vector<T> > { static std::string name() { return std::string("list<") + JceClass<T>::name() + ">"; } };
template<typename T, typename U> struct JceClass<std::map<T, U> > { static std::string name() { return std::string("map<") + JceClass<T>::name() + "," + JceClass<U>::name() + ">"; } };

namespace jce
{
        // is_convertible
        template<int N> struct type_of_size { char elements[N]; };

        typedef type_of_size<1>     yes_type;
        typedef type_of_size<2>     no_type;

        namespace meta_detail
        {
        struct any_conversion
        {
            template <typename T> any_conversion(const volatile T&);
            template <typename T> any_conversion(T&);
        };

        template <typename T> struct conversion_checker
        {
            static no_type _m_check(any_conversion ...);
            static yes_type _m_check(T, int);
        };
        }

        template<typename From, typename To> 
        class is_convertible
        {
            static From _m_from;
        public:
            enum { value = sizeof( meta_detail::conversion_checker<To>::_m_check(_m_from, 0) ) == sizeof(yes_type) };
        };

        template<typename T> 
        struct type2type { typedef T type; };

        template<typename T, typename U> 
        struct is_same_type
        {
            enum { value = is_convertible< type2type<T>, type2type<U> >::value };
        };

        // enable_if
        template<bool B, class T = void> struct enable_if_c { typedef T type; };

        template<class T> struct enable_if_c<false, T> {};

        template<class Cond, class T = void> 
        struct enable_if : public enable_if_c<Cond::value, T> {};

        template<bool B, class T = void> struct disable_if_c { typedef T type; };

        template<class T> struct disable_if_c<true, T> {};

        template<class Cond, class T = void> 
        struct disable_if : public disable_if_c<Cond::value, T> {};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_JCE_COPY_STRUCT_1(x, y, s)               \
    inline void jce_copy_struct_impl(x& a, const y& b)  { s; }  \
    inline void jce_copy_struct_impl(y& a, const x& b)  { s; }  \
    inline void jce_copy_struct(x& a, const y& b)  { jce_copy_struct_impl(a, b); }      \
    inline void jce_copy_struct(y& a, const x& b)  { jce_copy_struct_impl(a, b); }
 
#define DEFINE_JCE_COPY_STRUCT(n, x, y)                 \
    DEFINE_JCE_COPY_STRUCT_1(n::x, y, n##_##x##_JCE_COPY_STRUCT_HELPER)

inline void jce_copy_struct(char& a, const unsigned char& b) { a = b; }
inline void jce_copy_struct(unsigned char& a, const char& b) { a = b; }
template<typename T> inline void jce_copy_struct(T& a, const T& b) { a = b; }

template<typename T, typename U>
inline void jce_copy_struct(std::vector<T>& a, const std::vector<U>& b, typename jce::disable_if<jce::is_same_type<T, U>, void ***>::type dummy = 0)
{
    a.resize(b.size());
    for(size_t i = 0; i < a.size(); ++i)
        jce_copy_struct_impl(a[i], b[i]);
}

template<typename K1, typename V1, typename K2, typename V2>
inline void jce_copy_struct(std::map<K1, V1>& a, const std::map<K2, V2>& b, typename jce::disable_if<jce::is_same_type<std::map<K1, V1>, std::map<K2, V2> >, void ***>::type dummy = 0)
{
    a.clear();
    std::pair<K1, V1> pr;
    typedef typename std::map<K2, V2>::const_iterator IT;
    IT ib = b.begin(), ie = b.end();
    for(; ib != ie; ++ib){
        jce_copy_struct_impl(pr.first, ib->first);
        jce_copy_struct_impl(pr.second, ib->second);
        a.insert(pr);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
#endif
