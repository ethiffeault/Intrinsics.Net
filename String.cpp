//  MIT License
//  
//  Copyright(c) 2017 Eric Thiffeault
//  
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files(the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions :
//  
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.

#include "String.h"

#include <vcclr.h>          // cli/c++ pinning
#include <intrin.h>         // intrinsics
#include <emmintrin.h>      // SSE2
#include <immintrin.h>      // AVX2
#include "InstructionSet.h" // cpu intrinsics support helper

// add the check here, calling InstructionSet::SSE2() inside managed code is very slow due to bit manipulation
static const bool CpuSupportSse2 = InstructionSet::SSE2();
// untested yet, should work but my I7 don't support it so I cannot valid it
static const bool CpuSupportAvx2 = false; // InstructionSet::AVX2();

#pragma unmanaged

int StrIndexOfAll_SSE2(const wchar_t * str, const wchar_t* chars, int charsLength, int startIndex, int count, int* results)
{
    int* resultCur = results;
    const wchar_t* s = str + startIndex;
    const wchar_t* end = s + count;

    __m128i zero = _mm_setzero_si128();
    __m128i chars128[Intrinsics::SearchCharsMax];
    __m128i charsIndex128[Intrinsics::SearchCharsMax];

    for (int i = 0; i < charsLength; ++i)
    {
        chars128[i] = _mm_set1_epi16(chars[i]);
        charsIndex128[i] = _mm_set1_epi16(i);
    }

    __m128i mergeCompare = zero;
    __m128i mergeIndex = zero;

    // don't use unalign load here, sse2 code here was slower then the c++ version on x64
    for (; s < end && (size_t)s & (__alignof(__m128i) - 1); ++s)
    {
        for (int i = 0; i < charsLength; ++i)
        {
            const wchar_t c = chars[i];
            if (*s == c)
            {
                int index = (int)(s - str);
                *(resultCur++) = index;   // string index in str
                *(resultCur++) = i;       // char index in chars
                break;
            }
        }
    }

    // process aligned string part
    __m128i store;
    const wchar_t* alignEnd = end - 8;
    for (; s < alignEnd; s += 8)
    {
        __m128i  str128 = _mm_load_si128((__m128i const *)s);

        for (int i = 0; i < charsLength; ++i)
        {
            __m128i  cmp = _mm_cmpeq_epi16(chars128[i], str128);
            __m128i  cmpIndex = _mm_and_si128(cmp, charsIndex128[i]);
            mergeCompare = _mm_or_si128(mergeCompare, cmp);
            mergeIndex = _mm_or_si128(mergeIndex, cmpIndex);
        }

        unsigned v0 = _mm_movemask_epi8(mergeCompare);
        if (v0)
        {
            do
            {
                unsigned long traillingZero;
                _BitScanForward(&traillingZero, v0);
                const int offset = (traillingZero >> 1);
                const wchar_t* c = s + offset;
                *(resultCur++) = (int)(c - str);                // string index in str
                _mm_store_si128((__m128i*)&store, mergeIndex);
                *(resultCur++) = store.m128i_i16[offset];
                v0 &= ~(0x3 << traillingZero);                  // clear result char
            } while (v0);

            mergeCompare = zero;
            mergeIndex = zero;
        }
    }

    // process remaining string
    for (; s < end; ++s)
    {
        for (int i = 0; i < charsLength; ++i)
        {
            const wchar_t c = chars[i];
            if (*s == c)
            {
                int index = (int)(s - str);
                *(resultCur++) = index;   // string index in str
                *(resultCur++) = i;       // char index in chars
                break;
            }
        }
    }
    return (int)(resultCur - results) >> 1;
}

#ifdef INTRINSICS_TEST
int StrIndexOfAll_SSE2_V2(const wchar_t * str, const wchar_t* chars, int charsLength, int startIndex, int count, int* results)
{
    return StrIndexOfAll_SSE2(str, chars, charsLength, startIndex, count, results);
}
#endif

// not tested!
int StrIndexOfAll_AVX2(const wchar_t * str, const wchar_t* chars, int charsLength, int startIndex, int count, int* results)
{
    int* resultCur = results;
    const wchar_t* s = str + startIndex;
    const wchar_t* end = s + count;

    // process begin of string, unalign part
    if ((size_t)s & (__alignof(__m256i) - 1))
    {
        const int unalignCount = (__alignof(__m256i) - (((uintptr_t)s) & (__alignof(__m256i) - 1))) >> 1;
        const wchar_t* unalignEnd = s + (unalignCount < count ? unalignCount : count);
        for (; s < unalignEnd; ++s)
        {
            for (int i = 0; i < charsLength; ++i)
            {
                if (*s == chars[i])
                {
                    *(resultCur++) = (int)(s - str);    // string index in str
                    *(resultCur++) = i;                 // char index in chars
                }
            }
        }
        if (unalignEnd == end)
            return (int)(resultCur - results) >> 1;
    }

    __m256i zero = _mm256_setzero_si256();
    __m256i chars128[Intrinsics::SearchCharsMax];
    __m256i charsIndex128[Intrinsics::SearchCharsMax];

    for (int i = 0; i < charsLength; ++i)
    {
        chars128[i] = _mm256_set1_epi16(chars[i]);
        charsIndex128[i] = _mm256_set1_epi16(i);
    }

    // sse process aligned string part
    __m256i mergeCompare = zero;
    __m256i mergeIndex = zero;

    for (; s + 16 < end; s += 16)
    {
        __m256i  str128 = _mm256_loadu_si256((__m256i const *)s);

        for (int i = 0; i < charsLength; ++i)
        {
            __m256i  cmp = _mm256_cmpeq_epi16(chars128[i], str128);
            __m256i  cmpIndex = _mm256_and_si256(cmp, charsIndex128[i]);
            mergeCompare = _mm256_or_si256(mergeCompare, cmp);
            mergeIndex = _mm256_or_si256(mergeIndex, cmpIndex);
        }

        unsigned v0 = _mm256_movemask_epi8(mergeCompare);
        if (v0)
        {
            do
            {
                unsigned long traillingZero;
                _BitScanForward(&traillingZero, v0);
                const int offset = (traillingZero >> 1);
                const wchar_t* c = s + offset;
                *(resultCur++) = (int)(c - str);                       // string index in str
                *(resultCur++) = mergeIndex.m256i_i16[offset];  // char index in chars
                v0 &= ~(0x3 << traillingZero);                  // clear found char
            } while (v0);

            mergeCompare = zero;
            mergeIndex = zero;
        }
    }

    // process remaining string
    for (; s < end; ++s)
    {
        for (int i = 0; i < charsLength; ++i)
        {
            if (*s == chars[i])
            {
                *(resultCur++) = (int)(s - str);    // string index in str
                *(resultCur++) = i;                 // char index in chars
            }
        }
    }
    return (int)(resultCur - results) >> 1;
}

int StrIndexOfAll_CPP(const wchar_t * str, const wchar_t* chars, int charsLength, int startIndex, int count, int* results)
{
    int* resultCur = results;
    const wchar_t* s = str + startIndex;
    const wchar_t* end = s + count;

    for (; s < end; ++s)
    {
        for (int i = 0; i < charsLength; ++i)
        {
            const wchar_t c = chars[i];
            if (*s == c)
            {
                int index = (int)(s - str);
                *(resultCur++) = index;   // string index in str
                *(resultCur++) = i;       // char index in chars
                break;
            }
        }
    }

    return (int)(resultCur - results) >> 1;
}

int StrIndexOfAny_SSE2(const wchar_t * str, const wchar_t* chars, int charsLength, int startIndex, int count)
{
    __m128i zero = _mm_setzero_si128();
    __m128i chars128[Intrinsics::SearchCharsMax];
    __m128i mergeCompare = zero;
    for (int i = 0; i < charsLength; ++i)
        chars128[i] = _mm_set1_epi16(chars[i]);

    const wchar_t* s = str + startIndex;
    const wchar_t* end = s + count;
    for (; s < end && (size_t)s & (__alignof(__m128i) - 1); ++s)
    {
        for (int i = 0; i < charsLength; ++i)
        {
            const wchar_t c = chars[i];
            if (*s == c)
                return (int)(s - str);
        }
    }
    if (s == end)
        return -1;

    // process aligned string part
    const wchar_t* alignEnd = end - 8;
    for (; s < alignEnd; s += 8)
    {
        __m128i  str128 = _mm_load_si128((__m128i const *)s);

        for (int i = 0; i < charsLength; ++i)
        {
            __m128i  cmp = _mm_cmpeq_epi16(chars128[i], str128);
            mergeCompare = _mm_or_si128(mergeCompare, cmp);
        }

        unsigned v0 = _mm_movemask_epi8(mergeCompare);
        if (v0)
        {
            unsigned long traillingZero;
            _BitScanForward(&traillingZero, v0);
            const int offset = (traillingZero >> 1);
            const wchar_t* c = s + offset;
            return (int)(c - str);
        }
    }

    // process remaining string
    for (; s < end; ++s)
    {
        for (int i = 0; i < charsLength; ++i)
        {
            const wchar_t c = chars[i];
            if (*s == c)
                return (int)(s - str);
        }
    }

    return -1;
}


int StrIndexOfAny_CPP(const wchar_t * str, const wchar_t* chars, int charsLength, int startIndex, int count)
{
    const wchar_t* s = str + startIndex;
    const wchar_t* end = s + count;
    for (; s < end; ++s)
    {
        for (int i = 0; i < charsLength; ++i)
        {
            if (*s == chars[i])
                return (int)(s - str);
        }
    }
    return -1;
}

#pragma managed

namespace Intrinsics
{
    bool __clrcall String::IndexOfAll(System::String ^ str, wchar_t c, array<MatchIndex >^% results, [Out] int% resultsCount)
    {
        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        resultsCount = 0;
        int startIndex = 0;
        int count = str->Length;

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, wchar_t c, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex)
    {
        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        int count = str->Length - startIndex;

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, wchar_t c, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count)
    {
        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, &c, 1, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, array<wchar_t>^ chars, array<MatchIndex >^% results, [Out] int% resultsCount)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        resultsCount = 0;
        int startIndex = 0;
        int count = str->Length;

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = &chars[0];
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, array<wchar_t>^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        int count = str->Length - startIndex;

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = &chars[0];
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, array<wchar_t>^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = &chars[0];
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        resultsCount = 0;
        int startIndex = 0;
        int count = str->Length;

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = PtrToStringChars(chars);
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        int count = str->Length - startIndex;

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = PtrToStringChars(chars);
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = PtrToStringChars(chars);
        pin_ptr<MatchIndex > pinResults = &results[0];

        if (CpuSupportAvx2)
            resultsCount = StrIndexOfAll_AVX2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else if (CpuSupportSse2)
            resultsCount = StrIndexOfAll_SSE2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        else
            resultsCount = StrIndexOfAll_CPP(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    int __clrcall String::IndexOfAny(System::String ^ str, array<wchar_t>^ anyOf)
    {
        if (anyOf == nullptr)
            throw gcnew ArgumentNullException("anyOf is null");

        if (anyOf->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
            return -1;

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = &anyOf[0];
        return StrIndexOfAny_SSE2(pinStr, pinChars, anyOf->Length, 0, str->Length);
    }

    int __clrcall String::IndexOfAny(System::String ^ str, array<wchar_t>^ anyOf, int startIndex)
    {
        if (anyOf == nullptr)
            throw gcnew ArgumentNullException("anyOf is null");

        if (anyOf->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
            return -1;

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = &anyOf[0];
        return StrIndexOfAny_SSE2(pinStr, pinChars, anyOf->Length, startIndex, str->Length - startIndex);
    }

    int __clrcall String::IndexOfAny(System::String ^ str, array<wchar_t>^ anyOf, int startIndex, int count)
    {
        //if (str->Length < 128)
        //    return str->IndexOfAny(anyOf, startIndex, count);
        if (anyOf == nullptr)
            throw gcnew ArgumentNullException("anyOf is null");

        if (anyOf->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
            return -1;

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = &anyOf[0];
        return StrIndexOfAny_SSE2(pinStr, pinChars, anyOf->Length, startIndex, count);
    }

#ifdef INTRINSICS_TEST

    bool __clrcall String::IndexOfAllWip(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }


        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = PtrToStringChars(chars);
        pin_ptr<MatchIndex > pinResults = &results[0];

        resultsCount = StrIndexOfAll_SSE2_V2(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAllCli(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        resultsCount = 0;
        int end = startIndex + count;
        for (int i = startIndex; i < end; ++i)
        {
            for (int j = 0; j < chars->Length; ++j)
            {
                if (str[i] == chars[j])
                {
                    results[resultsCount].StringIndex = i;
                    results[resultsCount++].CharIndex = j;
                    break;
                }
            }
        }
        return resultsCount != 0;
    }

    bool __clrcall String::IndexOfAllCpp(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count)
    {
        if (chars->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
        {
            resultsCount = 0;
            return false;
        }

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        // realloc the to maximum possible results size if needed
        if (results->Length < str->Length)
            results = gcnew array<MatchIndex >(str->Length);

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = PtrToStringChars(chars);
        pin_ptr<MatchIndex > pinResults = &results[0];

        resultsCount = StrIndexOfAll_CPP(pinStr, pinChars, chars->Length, startIndex, count, (int*)pinResults);
        return resultsCount != 0;
    }


    int __clrcall String::IndexOfAnyCli(System::String ^ str, array<wchar_t>^ anyOf, int startIndex, int count)
    {
        if (anyOf == nullptr)
            throw gcnew ArgumentNullException("anyOf is null");

        if (anyOf->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
            return -1;

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        int end = startIndex + count;
        for (int i = startIndex; i < end; ++i)
        {
            for (int j = 0; j < anyOf->Length; ++j)
            {
                if (str[i] == anyOf[j])
                    return i;
            }
        }
        return -1;
    }

    int __clrcall String::IndexOfAnyCpp(System::String ^ str, array<wchar_t>^ anyOf, int startIndex, int count)
    {
        if (anyOf == nullptr)
            throw gcnew ArgumentNullException("anyOf is null");

        if (anyOf->Length > SearchCharsMax)
            throw gcnew ArgumentOutOfRangeException(System::String::Format(L"chars length must be smaller than {0}", SearchCharsMax));

        if (!str->Length)
            return -1;

        if (startIndex < 0 || startIndex + 1 > str->Length)
            throw gcnew ArgumentOutOfRangeException(L"startIndex must be greater than 0 and smaller than str length - 1");

        if (count > str->Length - startIndex)
            throw gcnew ArgumentOutOfRangeException(L"count must be smaller than str - startIndex");

        pin_ptr<const wchar_t> pinStr = PtrToStringChars(str);
        pin_ptr<const wchar_t> pinChars = &anyOf[0];
        return StrIndexOfAny_CPP(pinStr, pinChars, anyOf->Length, startIndex, count);
    }

#endif //#ifdef INTRINSICS_TEST
}
