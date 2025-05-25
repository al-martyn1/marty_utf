#pragma once

#include "defs.h"

//
#include <string>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <iterator>
#include <vector>

#include <cstdint>
#include <cwchar>


// BOM: 
// 0xFFFE    - UTF-18 little endian
// 0xFEFF    - UTF-18 big endian
// EF BB BF  - UTF-8

// https://ru.wikipedia.org/wiki/UTF-16
// https://ru.wikipedia.org/wiki/UTF-8
// https://habr.com/ru/post/138173/
// https://habr.com/ru/post/544084/
// https://habr.com/ru/post/485148/
// https://ilyabirman.ru/typography-layout/
// https://datatracker.ietf.org/doc/html/rfc2279

// Unicode exploit - https://habr.com/ru/post/126198/


// marty::utf::
namespace marty {
namespace utf {
} // namespace utf
} // namespace marty

namespace marty_utf = marty::utf;


namespace marty {
namespace utf {


typedef std::uint32_t    unicode_char_t;
typedef unicode_char_t   utf32_char_t  ;
typedef std::uint16_t    utf16_char_t  ;
typedef std::uint8_t     utf8_char_t   ;


//! Тип BOM - Byte Order Mark
enum class EBom
{
    noBom    = 0,   // 
    utf16le  = 1,   // 0xFF 0xFE
    utf16be  = 2,   // 0xFE 0xFF
    utf8     = 3    // 0xEF 0xBB 0xBF
};

//! Возвращает длину BOM
inline
std::size_t getBomLen(EBom eb)
{
    switch(eb)
    {
        case EBom::utf16le: return 2;
        case EBom::utf16be: return 2;
        case EBom::utf8   : return 3;
        case EBom::noBom  : [[fallthrough]];
        default      : return 0;
    }
}

//! Определяет тип BOM
inline
EBom detectBom(const utf8_char_t *pBegin, const utf8_char_t *pEnd)
{
    if (pBegin==pEnd)
        return EBom::noBom;

    // utf16le  = 1,   // 0xFF 0xFE
    // utf16be  = 2,   // 0xFE 0xFF
    // utf8     = 3    // 0xEF 0xBB 0xBF

    if (*pBegin==0xFF)
    {
        // maybe utf16le
        ++pBegin;
        if (pBegin==pEnd || *pBegin!=0xFE)
            return EBom::noBom;

        return EBom::utf16le;
    }
    else if (*pBegin==0xFE)
    {
        // maybe utf16be
        ++pBegin;
        if (pBegin==pEnd || *pBegin!=0xFF)
            return EBom::noBom;

        return EBom::utf16be;
    }
    else if (*pBegin==0xEF)
    {
        // maybe utf8
        ++pBegin;
        if (pBegin==pEnd || *pBegin!=0xBB)
            return EBom::noBom;

        ++pBegin;
        if (pBegin==pEnd || *pBegin!=0xBF)
            return EBom::noBom;
    
        return EBom::utf8;
    }

    return EBom::noBom;
}

//! Определяет тип BOM
inline
EBom detectBom(const std::string &str)
{
    if (str.empty())
        return EBom::noBom;

    const utf8_char_t *pBegin = (const utf8_char_t*)str.data();

    return detectBom(pBegin, pBegin+str.size());
}


//! Исключение - ошибка конвертации в/из кодировки юникода
class unicode_convert_error : public std::runtime_error // exception
{

  typedef std::runtime_error base_exception_t;

public:

    std::size_t        position; //!< Erroneous position

    unicode_convert_error(std::size_t p, const char *msg = "Unicode convert error") : base_exception_t(msg), position(p) {}

}; // class convert_error



//! Утилиты для определения value_type по типу итератора
namespace utils {

template<class OutputIterator>
struct detected_value_type
{
    using type = typename OutputIterator::value_type;
};

template<class Container>
struct detected_value_type< std::back_insert_iterator<Container> >
{
    using type = typename Container::value_type;
};

template<typename T>
using detected_value_type_t = typename detected_value_type<T>::type;


} // namespace utils
// template<typename OutputIterator>


//! Обмен байтов в слове
inline
utf16_char_t byteSwap(utf16_char_t b)
{
    return (utf16_char_t)((((b)&0xFF)<<8) | ((b>>8)&0xFF));
}

//! Обмен байтов в слове, условный
inline
utf16_char_t byteSwapEx(utf16_char_t ch, bool swapBytes)
{
    return swapBytes ? byteSwap(ch) : ch;
}


#if WCHAR_MAX <= 0xFFFFu /* wchar_t is 16 bit width, signed or unsigned */

    inline
    std::wstring wstring_from_utf16( const std::basic_string<utf16_char_t> &str16 )
    {
        if (str16.empty())
            return std::wstring();

        return std::wstring( (const wchar_t*)str16.data(), str16.size() );
    }

    inline
    std::basic_string<utf16_char_t> utf16_from_wstring( std::wstring const &str )
    {
        if (str.empty())
            return std::basic_string<utf16_char_t>();

        return std::basic_string<utf16_char_t>( (const utf16_char_t*)str.data(), str.size() );
    }

#else /* wchar_t is 32 bit width */

    inline
    std::wstring wstring_from_utf32( const std::basic_string<utf32_char_t> &str32 )
    {
        if (str32.empty())
            return std::wstring();

        return std::wstring( (const wchar_t*)str32.data(), str32.size() );
    }

    inline
    std::basic_string<utf32_char_t> utf32_from_wstring( std::wstring const &str )
    {
        if (str.empty())
            return std::basic_string<utf32_char_t>();

        return std::basic_string<utf32_char_t>( (const utf32_char_t*)str.data(), str.size() );
    }

#endif



inline
std::basic_string<utf32_char_t> utf32_from_utf16( const utf16_char_t *pBegin, const utf16_char_t *pEnd, bool swapBytes = false)
{
    std::basic_string<utf32_char_t> strRes;
    strRes.reserve((std::size_t)(pEnd-pBegin));

    const utf16_char_t *pChar = pBegin;

    // https://ru.wikipedia.org/wiki/UTF-16
    while(pChar!=pEnd)
    {
        utf16_char_t ch = byteSwapEx(*pChar++, swapBytes);
    
        if (ch<0xD800u || ch>0xDFFFu)
        {
            strRes.append(1, (utf32_char_t)ch);
        }
        else if (ch>=0xDC00u)
        {
            #ifdef UMBA_DEBUGBREAK
                UMBA_DEBUGBREAK();
            #endif
            throw unicode_convert_error((std::size_t)(pChar-pBegin), swapBytes ? "Invalid code sequence in UTF-16 with byte swap" : "Invalid code sequence in UTF-16");
        }
        else
        {
            utf32_char_t u32ch = ((utf32_char_t)(ch&0x03FFu)) << 10;
            if (pChar==pEnd)
            {
                #ifdef UMBA_DEBUGBREAK
                    UMBA_DEBUGBREAK();
                #endif
                throw unicode_convert_error((std::size_t)(pChar-pBegin), "Invalid code sequence in UTF-16 - unexpected end of data");
            }

            utf16_char_t ch2 = byteSwapEx(*pChar++, swapBytes);
           
            if (ch2<0xDC00u || ch2>0xDFFFu)
            {
                #ifdef UMBA_DEBUGBREAK
                    UMBA_DEBUGBREAK();
                #endif
                throw unicode_convert_error((std::size_t)(pChar-pBegin), swapBytes ? "Invalid code sequence in UTF-16 with byte swap (pair second)" : "Invalid code sequence in UTF-16 (pair second)");
            }

            strRes.append(1, (u32ch | (utf32_char_t)(ch2&0x03FFu)) );
        }

    } // while(pChar!=pEnd)

    return strRes;

}


#if WCHAR_MAX <= 0xFFFFu /* wchar_t is 16 bit width, signed or unsigned */

    inline
    std::basic_string<utf32_char_t> utf32_from_utf16( const std::wstring &wStr, bool swapBytes = false)
    {
        if (wStr.empty())
            return std::basic_string<utf32_char_t>();

        const utf16_char_t *pBegin = (const utf16_char_t*)wStr.data();
        return utf32_from_utf16(pBegin, pBegin+wStr.size(), swapBytes);
    }

#endif


inline
std::basic_string<utf16_char_t> utf16_from_utf32( const utf32_char_t *pBegin, const utf32_char_t *pEnd, bool swapBytes = false )
{
    std::basic_string<utf16_char_t> strRes;
    strRes.reserve((std::size_t)(pEnd-pBegin));

    const utf32_char_t *pChar = pBegin;

    // https://ru.wikipedia.org/wiki/UTF-16
    while(pChar!=pEnd)
    {
        if (*pChar < 0x10000)
        {
            utf16_char_t ch16 = (utf16_char_t)*pChar++;
            strRes.append(1, byteSwapEx(ch16, swapBytes));
        }
        else
        {
            utf16_char_t hiCh16 = ((utf16_char_t)(*pChar>>10))&0x03FFu;
            utf16_char_t loCh16 = ((utf16_char_t)(*pChar    ))&0x03FFu;
            ++pChar;
            strRes.append(1, byteSwapEx(0xD800u|hiCh16, swapBytes));
            strRes.append(1, byteSwapEx(0xDC00u|loCh16, swapBytes));
        }
    }

    return strRes;
}

inline
std::basic_string<utf16_char_t> utf16_from_utf32( const std::basic_string<utf32_char_t> &str32, bool swapBytes = false )
{
    if (str32.empty())
        return std::basic_string<utf16_char_t>();

    const utf32_char_t *pBegin = str32.data();
    return utf16_from_utf32(pBegin, pBegin+str32.size(), swapBytes);
}


// https://datatracker.ietf.org/doc/html/rfc2279

// UCS-4 range (hex.)    UTF-8 octet sequence (binary)
// 0000 0000-0000 007F   0xxxxxxx                                        0x80 == 0x00
// 0000 0080-0000 07FF   110xxxxx 10xxxxxx                               0xE0 == 0xC0
// 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx                      0xF0 == 0xE0
// 0001 0000-001F FFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx             0xF8 == 0xF0
// 0020 0000-03FF FFFF   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx    0xFC == 0xF8
// 0400 0000-7FFF FFFF   1111110x 10xxxxxx ... 10xxxxxx                  0xFE == 0xFC

constexpr
inline bool utfCheckMaskedValue(utf8_char_t ch, utf8_char_t mask, utf8_char_t value)
{
    utf8_char_t masked = utf8_char_t(ch&mask);
    return masked==value;
}

constexpr inline
bool isFirstCharUtf32(utf32_char_t ch)
{
    (void)(ch);
    return true;
}

constexpr inline
bool isNextCharUtf32(utf32_char_t ch)
{
    (void)(ch);
    return false;
}

constexpr inline
std::size_t getNumberOfCharsUtf32(utf32_char_t ch)
{
    (void)(ch);
    return 1;
}

constexpr inline
bool isFirstCharUtf16(utf16_char_t ch)
{
           // Обычный символ             // Первый символ сурогатной пары
    return (ch<0xD800u || ch>0xDFFFu) || (ch>=0xD800u && ch<=0xDBFFu);
}

constexpr inline
bool isNextCharUtf16(utf16_char_t ch)
{
    return !isFirstCharUtf16(ch);
}

constexpr inline
std::size_t getNumberOfCharsUtf16(utf16_char_t ch)
{
    return (ch<0xD800u || ch>0xDFFFu)
         ? 1u
         : ( (ch>=0xD800u && ch<=0xDBFFu)
           ? 2u
           : 0u
           )
         ;
}

constexpr
inline
std::size_t getNumberOfBytesUtf8(utf8_char_t ch)
{
    return (utfCheckMaskedValue(ch,0x80,0))
         ? 1u
         : ( (utfCheckMaskedValue(ch,0xE0,0xC0))
           ? 2u
           : ( (utfCheckMaskedValue(ch,0xF0,0xE0))
             ? 3u
             : ( (utfCheckMaskedValue(ch,0xF8,0xF0))
               ? 4u
               : ( (utfCheckMaskedValue(ch,0xFC,0xF8))
                 ? 5u
                 : ( (utfCheckMaskedValue(ch,0xFE,0xFC))
                   ? 6u
                   : 0u
                   )
                 )
               )
             )
           )
         ;
}

constexpr
inline
std::size_t getNumberOfBytesUtf8(char ch)
{
    return getNumberOfBytesUtf8(utf8_char_t(ch));
}

constexpr
inline
std::size_t getNumberOfCharsUtf8(utf8_char_t ch)
{
    return getNumberOfBytesUtf8(utf8_char_t(ch));
}

constexpr
inline
std::size_t getNumberOfCharsUtf8(char ch)
{
    return getNumberOfBytesUtf8(utf8_char_t(ch));
}

constexpr inline
bool isNextCharUtf8(utf8_char_t ch)
{
    return (ch&0xC0u)==0x80u; // 0xC0 - 0b1100_0000, 0x80 - 0b1000_0000
}

constexpr inline
bool isFirstCharUtf8(utf8_char_t ch)
{
    return !isNextCharUtf8(ch);
}

template<typename CharType> constexpr 
std::size_t getNumberOfCharsUtf(CharType ch)
{
    if constexpr (sizeof(ch)<=1)
    {
        return getNumberOfCharsUtf8((utf8_char_t)ch);
    }
    else
    {
        if constexpr (sizeof(ch)<=2)
            return getNumberOfCharsUtf16((utf16_char_t)ch);
        else
            return getNumberOfCharsUtf32((utf32_char_t)ch);
    }
}

template<typename CharType> constexpr 
bool isFirstCharUtf(CharType ch)
{
    if constexpr (sizeof(ch)<=1)
    {
        return isFirstCharUtf8((utf8_char_t)ch);
    }
    else
    {
        if constexpr (sizeof(ch)<=2)
            return isFirstCharUtf16((utf16_char_t)ch);
        else
            return isFirstCharUtf32((utf32_char_t)ch);
    }
}

template<typename CharType> constexpr 
bool isNextCharUtf(CharType ch)
{
    if constexpr (sizeof(ch)<=1)
    {
        return isNextCharUtf8((utf8_char_t)ch);
    }
    else
    {
        if constexpr (sizeof(ch)<=2)
            return isNextCharUtf16((utf16_char_t)ch);
        else
            return isNextCharUtf32((utf32_char_t)ch);
    }
}


inline
std::size_t getStringLenUtf8(const std::string &str)
{
    using marty_utf::utf8_char_t;

    std::size_t numSymbols = 0;

    std::size_t curPos = 0;

    while(curPos<str.size())
    {
        auto uch = (utf8_char_t)str[curPos];

        auto symbolNumBytes = getNumberOfBytesUtf8(uch);

        std::size_t nextPos = curPos + symbolNumBytes;

        if (nextPos<=str.size())
        {
            ++numSymbols;
        }

        curPos = nextPos;
    }

    return numSymbols;

}

template<typename CharType>
utf32_char_t utfCharsToUtf32(const CharType *pb, const CharType *pe)
{
    //constexpr 
    static const utf8_char_t firstByteMasks[6] = { 0x7Fu, 0x1Fu, 0x0Fu, 0x07u, 0x03u, 0x01u };

    using const_ptr_utf8_char_t = const utf8_char_t*;

    const_ptr_utf8_char_t pChar = const_ptr_utf8_char_t(pb);

    // UCS-4 range (hex.)    UTF-8 octet sequence (binary)
    // 1 - 0000 0000-0000 007F   0xxxxxxx
    // 2 - 0000 0080-0000 07FF   110xxxxx 10xxxxxx
    // 3 - 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
    // 4 - 0001 0000-001F FFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    // 5 - 0020 0000-03FF FFFF   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    // 6 - 0400 0000-7FFF FFFF   1111110x 10xxxxxx ... 10xxxxxx

    if (pb==pe)
        return utf32_char_t(0);

    utf8_char_t firstByteUtf8 = utf8_char_t(*pChar++);
    std::size_t numberOfBytesUtf8 = getNumberOfBytesUtf8(firstByteUtf8);
    if (!numberOfBytesUtf8)
    {
        return utf32_char_t(0);
    }

    numberOfBytesUtf8--;

    utf32_char_t ch32 = 0;
     
    utf8_char_t firstByteMask = 0;
    if (numberOfBytesUtf8<6)
        firstByteMask = firstByteMasks[numberOfBytesUtf8];
     
    if (!firstByteMask)
        return utf32_char_t(0);
     
    ch32 = (utf32_char_t)(firstByteUtf8&firstByteMask);
     
    auto i = 0u;
    for(; i!=numberOfBytesUtf8 && (pChar!= const_ptr_utf8_char_t(pe)); ++i, ++pChar)
    {
        ch32 <<= 6;
        ch32 |= (utf32_char_t)(*pChar&0x3F);
    }
     
    if (i==numberOfBytesUtf8)
    {
        // strRes.append(1, ch32); // complete symbol extracted
        if (ch32>0x10FFFFu)
        {
            // #ifdef UMBA_DEBUGBREAK
            //     UMBA_DEBUGBREAK();
            // #endif
            // throw unicode_convert_error((std::size_t)(pChar-pBegin), "Invalid code sequence in UTF-8 - symbol code is out of range (>0x10FFFFu)");
            return utf32_char_t(0);
        }
    }
    
    return ch32;
}

        //  
        // utf8_char_t firstByteUtf8 = *pChar++;
        // std::size_t numberOfBytesUtf8 = getNumberOfBytesUtf8(firstByteUtf8);
        // if (!numberOfBytesUtf8)
        // {
        //     continue;
        // }
        //  
        // numberOfBytesUtf8--;
        //  
        // utf32_char_t ch32 = 0;
        //  
        // utf8_char_t firstByteMask = 0;
        // if (numberOfBytesUtf8<6)
        //     firstByteMask = firstByteMasks[numberOfBytesUtf8];
        //  
        // if (!firstByteMask)
        //     continue;
        //  
        // ch32 = (utf32_char_t)(firstByteUtf8&firstByteMask);
        //  
        // // switch(numberOfBytesUtf8)
        // // {
        // //     case 0: ch32 = (utf32_char_t)(firstByteUtf8&0x7Fu); break;
        // //     case 1: ch32 = (utf32_char_t)(firstByteUtf8&0x1Fu); break;
        // //     case 2: ch32 = (utf32_char_t)(firstByteUtf8&0x0Fu); break;
        // //     case 3: ch32 = (utf32_char_t)(firstByteUtf8&0x07u); break;
        // //     case 4: ch32 = (utf32_char_t)(firstByteUtf8&0x03u); break;
        // //     case 5: ch32 = (utf32_char_t)(firstByteUtf8&0x01u); break;
        // //     default:
        // //         continue;
        // // }
        //  
        // auto i = 0u;
        // for(; i!=numberOfBytesUtf8 && (pChar!=pEnd); ++i, ++pChar)
        // {
        //     ch32 <<= 6;
        //     ch32 |= (utf32_char_t)(*pChar&0x3F);
        // }
        //  
        // if (i==numberOfBytesUtf8)
        // {
        //     // strRes.append(1, ch32); // complete symbol extracted
        //     if (ch32>0x10FFFFu)
        //     {
        //         #ifdef UMBA_DEBUGBREAK
        //             UMBA_DEBUGBREAK();
        //         #endif
        //         throw unicode_convert_error((std::size_t)(pChar-pBegin), "Invalid code sequence in UTF-8 - symbol code is out of range (>0x10FFFFu)");
        //     }
        //  
        //     *pOutputIter++ = ch32;
        // }


//----------------------------------------------------------------------------
template<typename CharType>
struct UtfInputIterator
{

    using value_type         = CharType;
    using pointer_type       = CharType*;
    using const_pointer_type = const CharType*;

    using ctor_require_two_pointers = void; // check CTOR requires single pointer or two pointers


    UtfInputIterator& operator++() // pre-increment - инкрементировать, и вернуть себя
    {
        incImpl();
        return *this;
    }

    UtfInputIterator operator++(int)
    {
        auto copyOfThis = *this;
        incImpl();
        return copyOfThis;
    }

    bool operator==(UtfInputIterator other) const
    {
        return isIteratorEnd()==other.isIteratorEnd();
    }

    bool operator!=(UtfInputIterator other) const
    {
        return isIteratorEnd()!=other.isIteratorEnd();
    }

    UtfInputIterator() : m_ptr(0), m_ptrEnd(0) {}
    UtfInputIterator(const UtfInputIterator &other) = default;
    UtfInputIterator(UtfInputIterator &&other) = default;
    UtfInputIterator& operator=(const UtfInputIterator &other) = default;
    UtfInputIterator& operator=(UtfInputIterator &&other) = default;


    template<typename OtherCharType>
    UtfInputIterator(const OtherCharType* b, const OtherCharType* e) : m_ptr(const_pointer_type(b)), m_ptrEnd(const_pointer_type(e)) {}

    template<typename OtherCharType>
    UtfInputIterator(const OtherCharType* e) : m_ptr(const_pointer_type(e)), m_ptrEnd(const_pointer_type(e)) {}

    template<typename OtherCharType>
    UtfInputIterator(const OtherCharType* b, std::size_t sz) : m_ptr(const_pointer_type(b)), m_ptrEnd(m_ptr+std::ptrdiff_t(sz)) {}

    template<typename OtherCharType>
    UtfInputIterator(const std::basic_string<OtherCharType> & strIn ) : m_ptr(const_pointer_type(strIn   .data())), m_ptrEnd(m_ptr+std::ptrdiff_t(strIn   .size())) {}

    template<typename OtherCharType>
    UtfInputIterator(const std::basic_string<OtherCharType> *pStrIn )
    {
        if (!pStrIn)
        {
            m_ptr = 0; 
            m_ptrEnd = m_ptr+std::ptrdiff_t(pStrIn ->size());
        }
        else
        {
            m_ptr = const_pointer_type(pStrIn ->data());
            m_ptrEnd = m_ptr+std::ptrdiff_t(pStrIn ->size());
        }
    }

    template<typename OtherCharType>
    UtfInputIterator(const std::vector<OtherCharType>       & dataIn)
    {
        if (dataIn.empty())
        {
             m_ptr = 0;
             m_ptrEnd = 0;
        }
        else
        {
             m_ptr = const_pointer_type(dataIn  .data());
             m_ptrEnd = m_ptr+std::ptrdiff_t(dataIn  .size());
        }
    }

    template<typename OtherCharType>
    UtfInputIterator(const std::vector<OtherCharType>       *pDataIn)
    {
        if (!pDataIn || pDataIn->empty())
        {
             m_ptr = 0;
             m_ptrEnd = 0;
        }
        else
        {
             m_ptr = const_pointer_type(pDataIn->data());
             m_ptrEnd = m_ptr+std::ptrdiff_t(pDataIn->size());
        }
    }

    //const UtfInputIterator& operator*() const
    utf32_char_t operator*() const
    {
        return utfCharsToUtf32(m_ptr, m_ptrEnd);
    }

    // operator utf32_char_t() const
    // {
    //     return utfCharsToUtf32(m_ptr, m_ptrEnd);
    // }

    std::size_t getByteOffset() const
    {
        return byteOffset;
    }

    std::size_t getSymbolOffset() const
    {
        return symbolOffset;
    }

    const CharType* rawPtr() const
    {
        return m_ptr;
    }

    const CharType* begin() const
    {
        return m_ptr;
    }

    const CharType* end() const
    {
        return m_ptrEnd;
    }


protected:

    bool isIteratorEnd() const
    {
        return m_ptr==m_ptrEnd;
    }

    void incImpl()
    {
        std::size_t chLen = getNumberOfBytesUtf8(*m_ptr);
        if (chLen==0)
        {
            for(; m_ptr!=m_ptrEnd && !isFirstCharUtf(*m_ptr); ++m_ptr) { ++byteOffset; }
        }
        else
        {
            for(std::size_t i=0; i!=chLen && m_ptr!=m_ptrEnd; ++i, ++m_ptr) { ++byteOffset; }
        }

        ++symbolOffset;
    }

    // bool isFirstCharUtf(CharType ch)

    const CharType* m_ptr     = 0;
    const CharType* m_ptrEnd  = 0;

    std::size_t byteOffset   = 0;
    std::size_t symbolOffset = 0;
};

//-----------------------------------------------------------------------------
template<typename CharType>
const char* rawConstCharPtrFromIterator(UtfInputIterator<CharType> it)
{
   return reinterpret_cast<const char*>(it.rawPtr());
}

//-----------------------------------------------------------------------------
// std::size_t getNumberOfBytesUtf8(utf8_char_t ch)

//-----------------------------------------------------------------------------
//! Вычисляет длину символа в char'ах для однобайтных кодировок
struct SymbolLenCalculatorEncodingUtf8
{
    std::size_t operator()(const char *pCh) const
    {
        if (!pCh)
            return 1;
        return getNumberOfBytesUtf8((utf8_char_t)*pCh);
    }
};


inline
const utf8_char_t* utf8_find_first_symbol_byte(const utf8_char_t *pBegin, const utf8_char_t *pEnd)
{
    while( (pBegin!=pEnd) && ((*pBegin)&0xC0u)==0x80u ) // 0xC0 - 0b1100_0000, 0x80 - 0b1000_0000
    {
        ++pBegin;
    }

    return pBegin;
}

template<typename OutputIterator>
inline
void utf32_from_utf8_impl( const utf8_char_t *pBegin, const utf8_char_t *pEnd, OutputIterator pOutputIter )
{
    const utf8_char_t *pChar = pBegin;

    static const utf8_char_t firstByteMasks[6] = { 0x7Fu, 0x1Fu, 0x0Fu, 0x07u, 0x03u, 0x01u };

    // auto checkByteUtf8 = [&](const utf8_char_t *p)
    // {
    //     if (p==pEnd)
    //     {
    //         throw unicode_convert_error((std::size_t)(p-pBegin), "Invalid code sequence in UTF-8 - unexpected end of data");
    //     }
    // }

    while(pChar!=pEnd)
    {
        // Пытаемся найти стартовый символ - не падаем при ошибке, а синхронизируемся с потоком байтов

        pChar = utf8_find_first_symbol_byte(pChar,pEnd);

        if (pChar==pEnd)
        {
            continue;
        }

        // https://datatracker.ietf.org/doc/html/rfc2279
       
        // UCS-4 range (hex.)    UTF-8 octet sequence (binary)
        // 1 - 0000 0000-0000 007F   0xxxxxxx
        // 2 - 0000 0080-0000 07FF   110xxxxx 10xxxxxx
        // 3 - 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
        // 4 - 0001 0000-001F FFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        // 5 - 0020 0000-03FF FFFF   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        // 6 - 0400 0000-7FFF FFFF   1111110x 10xxxxxx ... 10xxxxxx

        utf8_char_t firstByteUtf8 = *pChar++;
        std::size_t numberOfBytesUtf8 = getNumberOfBytesUtf8(firstByteUtf8);
        if (!numberOfBytesUtf8)
        {
            continue;
        }

        numberOfBytesUtf8--;

        utf32_char_t ch32 = 0;

        utf8_char_t firstByteMask = 0;
        if (numberOfBytesUtf8<6)
            firstByteMask = firstByteMasks[numberOfBytesUtf8];

        if (!firstByteMask)
            continue;

        ch32 = (utf32_char_t)(firstByteUtf8&firstByteMask);

        // switch(numberOfBytesUtf8)
        // {
        //     case 0: ch32 = (utf32_char_t)(firstByteUtf8&0x7Fu); break;
        //     case 1: ch32 = (utf32_char_t)(firstByteUtf8&0x1Fu); break;
        //     case 2: ch32 = (utf32_char_t)(firstByteUtf8&0x0Fu); break;
        //     case 3: ch32 = (utf32_char_t)(firstByteUtf8&0x07u); break;
        //     case 4: ch32 = (utf32_char_t)(firstByteUtf8&0x03u); break;
        //     case 5: ch32 = (utf32_char_t)(firstByteUtf8&0x01u); break;
        //     default:
        //         continue;
        // }

        auto i = 0u;
        for(; i!=numberOfBytesUtf8 && (pChar!=pEnd); ++i, ++pChar)
        {
            ch32 <<= 6;
            ch32 |= (utf32_char_t)(*pChar&0x3F);
        }

        if (i==numberOfBytesUtf8)
        {
            // strRes.append(1, ch32); // complete symbol extracted
            if (ch32>0x10FFFFu)
            {
                #ifdef UMBA_DEBUGBREAK
                    UMBA_DEBUGBREAK();
                #endif
                throw unicode_convert_error((std::size_t)(pChar-pBegin), "Invalid code sequence in UTF-8 - symbol code is out of range (>0x10FFFFu)");
            }

            *pOutputIter++ = ch32;
        }

    }

}

inline
std::basic_string<utf32_char_t> utf32_from_utf8( const utf8_char_t *pBegin, const utf8_char_t *pEnd )
{
    std::basic_string<utf32_char_t> strRes;
    strRes.reserve(4u*(std::size_t)((pEnd-pBegin))); // Предполагаем худший случай
    utf32_from_utf8_impl( pBegin, pEnd, std::back_inserter(strRes) );
    strRes.shrink_to_fit();
    return strRes;
}

inline
std::basic_string<utf32_char_t> utf32_from_utf8( const std::basic_string<utf8_char_t> &str8)
{
    if (str8.empty())
        return std::basic_string<utf32_char_t>();

    const utf8_char_t *pBegin = str8.data();
    return utf32_from_utf8(pBegin, pBegin+str8.size());
}

inline
std::basic_string<utf32_char_t> utf32_from_utf8( const std::string &str8)
{
    if (str8.empty())
        return std::basic_string<utf32_char_t>();

    const utf8_char_t *pBegin = (const utf8_char_t*)str8.data();
    return utf32_from_utf8(pBegin, pBegin+str8.size());
}


#if WCHAR_MAX <= 0xFFFFu /* wchar_t is 16 bit width, signed or unsigned */

#else

inline
std::wstring wstring32_from_utf8( const utf8_char_t *pBegin, const utf8_char_t *pEnd )
{
    std::wstring strRes;
    strRes.reserve(4*(pEnd-pBegin)); // Предполагаем худший случай
    utf32_from_utf8_impl( pBegin, pEnd, std::back_inserter(strRes) );
    strRes.shrink_to_fit();
    return strRes;
}

inline
std::wstring wstring32_from_utf8( const std::basic_string<utf8_char_t> &str8)
{
    if (str8.empty())
        return std::wstring();

    const utf8_char_t *pBegin = str8.data();
    return wstring32_from_utf8(pBegin, pBegin+str8.size());
}

inline
std::wstring wstring32_from_utf8( const std::string &str8)
{
    if (str8.empty())
        return std::wstring();

    const utf8_char_t *pBegin = (const utf8_char_t*)str8.data();
    return utf32_from_utf8(pBegin, pBegin+str8.size());
}

#endif


template<typename OutputIterator>
inline
void utf8_from_utf32_impl(const utf32_char_t *pBegin, const utf32_char_t *pEnd, OutputIterator pOutputIter)
{

    // typedef typename std::iterator_traits<OutputIterator>::value_type  IterCharType;
    // using IterCharType = typename std::iterator_traits<OutputIterator>::value_type;
    using iterator_type = std::decay_t<decltype(pOutputIter)>;
    using value_type    = utils::detected_value_type_t< iterator_type >;


    // UCS-4 range (hex.)                    UTF-8 octet sequence (binary)
    // 1 - 0000 0000-0000 007F   0x00/0x7F   0xxxxxxx
    // 2 - 0000 0080-0000 07FF   0xC0/0x1F   110xxxxx 10xxxxxx
    // 3 - 0000 0800-0000 FFFF   0xE0/0x0F   1110xxxx 10xxxxxx 10xxxxxx
    // 4 - 0001 0000-001F FFFF   0xF0/0x07   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    // 5 - 0020 0000-03FF FFFF   0xF8/0x03   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    // 6 - 0400 0000-7FFF FFFF   0xFC/0x01   1111110x 10xxxxxx ... 10xxxxxx
    //                                                0x80/0x3F

    for(const utf32_char_t* pChar=pBegin; pChar!=pEnd; ++pChar)
    {
        utf32_char_t ch = *pChar;

        if (ch<=0x7Fu) // 1
        {
            *pOutputIter++ = (value_type)(utf8_char_t)(ch);
        }
        else if (ch<=0x7FFu) // 2
        {
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 6)&0x1F) | 0xC0);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 0)&0x3F) | 0x80);
        }
        else if (ch<=0xFFFFu) // 3
        {
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>12)&0x0F) | 0xE0);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 6)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 0)&0x3F) | 0x80);
        }
        else if (ch<=0x1FFFFFu) // 4 
        {
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>18)&0x07) | 0xF0);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>12)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 6)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 0)&0x3F) | 0x80);
        }
        else if (ch<=0x3FFFFFFu) // 5
        {
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>21)&0x03) | 0xF8);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>18)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>12)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 6)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 0)&0x3F) | 0x80);
        }
        else // 6
        {
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>24)&0x01) | 0xFC);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>21)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>18)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>>12)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 6)&0x3F) | 0x80);
            *pOutputIter++ = (value_type)(utf8_char_t)(((ch>> 0)&0x3F) | 0x80);
        }
    }

}

inline
std::basic_string<utf8_char_t> utf8_from_utf32(const utf32_char_t *pBegin, const utf32_char_t *pEnd)
{
    std::basic_string<utf8_char_t> strRes;
    strRes.reserve(4u*(std::size_t)((pEnd-pBegin))); // Предполагаем худший случай
    utf8_from_utf32_impl(pBegin, pEnd, std::back_inserter(strRes));
    strRes.shrink_to_fit();
    return strRes;
}

inline
std::string string_from_utf32(const utf32_char_t *pBegin, const utf32_char_t *pEnd)
{
    std::string strRes;
    strRes.reserve(4u*(std::size_t)((pEnd-pBegin))); // Предполагаем худший случай
    utf8_from_utf32_impl(pBegin, pEnd, std::back_inserter(strRes));
    strRes.shrink_to_fit();
    return strRes;
}

inline
std::string string_from_utf32(const std::basic_string<utf32_char_t> &str)
{
    // return string_from_utf32(str.begin(), str.end());
    // return string_from_utf32((const utf32_char_t*)str.begin(), (const utf32_char_t*)str.end());
    return string_from_utf32(&str.front(), &str.back()+1);
}


//----------------------------------------------------------------------------
template<typename CharType>
struct UtfOutputIterator
{
 
    UtfOutputIterator& operator++() // pre-increment - инкрементировать, и вернуть себя
    {
        return *this;
    }
 
    UtfOutputIterator operator++(int)
    {
        return *this;
    }
 
    bool operator==(UtfOutputIterator other) const
    {
        return false;
    }
 
    bool operator!=(UtfOutputIterator other) const
    {
        return true;
    }
 
    UtfOutputIterator() : m_pStr(0) {}
    UtfOutputIterator(const UtfOutputIterator &other) = default;
    UtfOutputIterator(UtfOutputIterator &&other) = default;
    UtfOutputIterator& operator=(const UtfOutputIterator &other) = default;
    UtfOutputIterator& operator=(UtfOutputIterator &&other) = default;
 
    UtfOutputIterator(std::basic_string<CharType> & strOut) : m_pStr(&strOut) {}
    UtfOutputIterator(std::basic_string<CharType> *pStrOut) : m_pStr(&pStrOut) {}

 
    UtfOutputIterator& operator*() 
    {
        return *this;
    }

    UtfOutputIterator& operator=(utf32_char_t ch32)
    {
        if (!m_pStr)
        {
            MARTY_UTF_ASSERT_FAIL("Underlaying string ptr is null, default CTOR used to create UtfOutputIterator?");
        }
        utf8_from_utf32_impl(&ch32, &ch32+1, std::back_inserter(*m_pStr));
        return *this;
    }
 
 
protected:
 
    std::basic_string<CharType> *m_pStr = 0;

}; // struct UtfOutputIterator



#if WCHAR_MAX <= 0xFFFFu /* wchar_t is 16 bit width, signed or unsigned */

inline
std::basic_string<utf8_char_t> utf8_from_wstring(const std::wstring &wstr)
{
    // Под виндой wstring 16ти битные
    // Сначала конвертируем в 32битный UTF

    auto strUtf32 = utf32_from_utf16((const utf16_char_t*)wstr.data(), (const utf16_char_t*)wstr.data()+std::ptrdiff_t(wstr.size()), false  /* swapBytes */ );
    return utf8_from_utf32(strUtf32.data(), strUtf32.data() + std::ptrdiff_t(wstr.size()));
}

inline
std::string string_from_wstring(const std::wstring &wstr)
{
    // Под виндой wstring 16ти битные
    // Сначала конвертируем в 32битный UTF

    auto strUtf32 = utf32_from_utf16((const utf16_char_t*)wstr.data(), (const utf16_char_t*)wstr.data()+std::ptrdiff_t(wstr.size()), false  /* swapBytes */ );
    return string_from_utf32(strUtf32.data(), strUtf32.data() + std::ptrdiff_t(wstr.size()));
}

#else /* wchar_t is 32 bit width */

inline
std::basic_string<utf8_char_t> utf8_from_wstring(const std::wstring &wstr)
{
    return utf8_from_utf32(wstr.data(), wstr.data()+std::ptrdiff_t(wstr.size()));
}

inline
std::string string_from_wstring(const std::wstring &wstr)
{
    return string_from_utf32(wstr.data(), wstr.data()+std::ptrdiff_t(wstr.size()));
}

#endif


// std::basic_string<utf8_char_t> utf8_from_utf32(const utf32_char_t *pBegin, const utf32_char_t *pEnd)
// std::string string_from_utf32(const utf32_char_t *pBegin, const utf32_char_t *pEnd)
// std::string string_from_utf32(const std::basic_string<utf32_char_t> &str)


// std::basic_string<utf32_char_t> utf32_from_utf16( const utf16_char_t *pBegin, const utf16_char_t *pEnd, bool swapBytes = false)


//TODO: !!! Надо бы сделать:
// UTF-32 из string.
// UTF-32 из wstring.
// string  из UTF-32.
// wstring из UTF-32.



//----------------------------------------------------------------------------
// Все о пробелах - https://prgssr.ru/development/vse-o-probelah.html
// Символы нулевой ширины (пробельные) - https://kirillbelyaev.com/ru/blog/?go=all/zero-width-characters/
// Zero-width non-joiner (предотвращает склеивание символов в лигатуры) - https://en.wikipedia.org/wiki/Zero-width_non-joiner
// Soft hyphen (выводит перенос на границах строк, не отображается внутри строки) - https://en.wikipedia.org/wiki/Soft_hyphen
// Combining character - https://en.wikipedia.org/wiki/Combining_character
//   Combining Diacritical Marks (0300–036F), since version 1.0, with modifications in subsequent versions down to 4.1
//   Combining Diacritical Marks Extended (1AB0–1AFF), version 7.0
//   Combining Diacritical Marks Supplement (1DC0–1DFF), versions 4.1 to 5.2
//   Combining Diacritical Marks for Symbols (20D0–20FF), since version 1.0, with modifications in subsequent versions down to 5.1
//   Cyrillic Extended-A (2DE0–2DFF), version 5.1
//   Combining Half Marks (FE20–FE2F), versions 1.0, with modifications in subsequent versions down to 8.0
//  
// Combining characters are assigned the Unicode major category "M" ("Mark").  
// Unicode character property - https://en.wikipedia.org/wiki/Unicode_character_property#General_Category

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// simpleUnicodeFeature - suf

//----------------------------------------------------------------------------
//!
inline
bool sufIsCombiningDiacretic(utf32_char_t ch)
{
    if (ch>=0x0300 && ch<=0x036F)
        return true;

    if (ch>=0x1AB0 && ch<=0x1AFF)
        return true;

    if (ch>=0x1DC0 && ch<=0x1DFF)
        return true;

    if (ch>=0x20D0 && ch<=0x20FF)
        return true;

    if (ch>=0x2DE0 && ch<=0x2DFF)
        return true;

    if (ch>=0xFE20 && ch<=0xFE2F)
        return true;

    // if (ch>=0x && ch<=0x)
    //     return true;

    return false;
}

//----------------------------------------------------------------------------
//!
inline
bool sufIsZeroWidthSpace(utf32_char_t ch)
{
    switch(ch)
    {
        case 0xFEFF: return true; // Zero Width No-break Space U+FEFF &#65279; // Устаревшее, используется сейчас только для детекта BOM
        case 0x2060: return true; // Word Joiner U+2060 &#8288; &NoBreak; // замена предыдущему // https://unicode-explorer.com/c/2060
        case 0x200B: return true; // Zero Width Space  U+200B  &#8203;  &NegativeMediumSpace;
        case 0x200D: return true; // Zero Width Joiner U+200D  &#8205;  &zwj;
        case 0x200C: return true; // Zero Width Non-joiner U+200C  &#8204;  &zwnj;
        default: return false;
    }
}

#if 0
//! Позволяют разделять слова, но могут быть нулевой ширины
inline
bool sufIsWordBreakingSpace(utf32_char_t ch)
{
    switch(ch)
    {
        case 0x20: return true;

        default: return false;
    }
}

//! Позволяет разделять слова, но при выводе не даёт разбить слова на несколько строк
inline
bool sufIsNonBreakingSpace(utf32_char_t ch)
{
    // Narrow No-Break Space (NNBSP) U+202F // https://www.compart.com/en/unicode/U+202F
    // No-Break Space (NBSP) U+00A0 // https://www.compart.com/en/unicode/U+00A0

         // case 0x2060: // Word Joiner U+2060 &#8288; &NoBreak; // https://unicode-explorer.com/c/2060

    return false;
}
#endif










} // namespace utf
} // namespace marty

// marty::utf::

