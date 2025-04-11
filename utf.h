#pragma once

#include "utf_impl.h"

// #include "marty_utf/utf.h"
namespace marty {
namespace utf {

// using marty_utf::toUtf8;
// using marty_utf::fromUtf8

inline
std::string toUtf8(const std::wstring &str)
{

    #if WCHAR_MAX <= 0xFFFFu /* wchar_t is 16 bit width, signed or unsigned */

        auto  utfCharsStr32        = utf32_from_utf16(str);
        const utf32_char_t *pBegin = utfCharsStr32.data();
        return string_from_utf32(pBegin, pBegin+utfCharsStr32.size());

    #else

        const utf32_char_t *pBegin = str.data();
        return string_from_utf32(pBegin, pBegin+str.size());

    #endif

    // auto pApi = getEncodingsApi();
    // return pApi->encode(str, EncodingsApi::cpid_UTF8);
}

inline
std::string toUtf8(const std::string &str)
{
    return str;
}


inline
std::string toUtf8(wchar_t ch)
{
    // auto wstr = std::wstring(1,ch);
    // return toUtf8(wstr);

    #if WCHAR_MAX <= 0xFFFFu /* wchar_t is 16 bit width, signed or unsigned */

        const utf16_char_t *pStrW  = (const utf16_char_t*)&ch;
        auto  utfCharsStr32        = utf32_from_utf16(pStrW, pStrW+1);
        const utf32_char_t *pBegin = utfCharsStr32.data();
        return string_from_utf32(pBegin, pBegin+utfCharsStr32.size());

    #else

        const utf32_char_t *pBegin = &ch;
        return string_from_utf32(pBegin, pBegin+1);

    #endif

}

inline
std::string toUtf8(utf32_char_t ch)
{
    const utf32_char_t *pBegin = &ch;
    return string_from_utf32(pBegin, pBegin+1);
}

inline
std::string toUtf8(char ch)
{
    return std::string(1,ch);
}

inline
std::wstring fromUtf8(const std::string &str)
{
    #if WCHAR_MAX <= 0xFFFFu /* wchar_t is 16 bit width, signed or unsigned */

        std::basic_string<utf32_char_t> stringU32 = utf32_from_utf8(str);
        std::basic_string<utf16_char_t> stringU16 = utf16_from_utf32(stringU32);
        return wstring_from_utf16(stringU16);

    #else

        return wstring32_from_utf8(str);

    #endif

    // auto pApi = getEncodingsApi();
    // return pApi->decode(str, EncodingsApi::cpid_UTF8);
}



} // namespace utf
} // namespace marty



