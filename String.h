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
#pragma once

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

namespace Intrinsics
{
    static const int SearchCharsMax = 32;

    public ref class String abstract sealed
    {
    public:

        value struct MatchIndex
        {
        public:
            __clrcall MatchIndex(int stringIndex, int charIndex)
            {
                StringIndex = stringIndex;
                CharIndex = charIndex;
            };

            int StringIndex;
            int CharIndex;
        };

        literal int SearchCharsMax = Intrinsics::SearchCharsMax;

        static bool __clrcall IndexOfAll(System::String ^ str, wchar_t c, array<MatchIndex >^% results, [Out] int% resultsCount);

        static bool __clrcall IndexOfAll(System::String ^ str, wchar_t c, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex);

        static bool __clrcall IndexOfAll(System::String ^ str, wchar_t c, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        static bool __clrcall IndexOfAll(System::String ^ str, array<wchar_t>^ c, array<MatchIndex >^% results, [Out] int% resultsCount);

        static bool __clrcall IndexOfAll(System::String ^ str, array<wchar_t>^ c, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex);

        static bool __clrcall IndexOfAll(System::String ^ str, array<wchar_t>^ c, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        static bool __clrcall IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount);

        static bool __clrcall IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex);

        static bool __clrcall IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        static int __clrcall IndexOfAny(System::String ^ str, array<wchar_t>^ anyOf);

        static int __clrcall IndexOfAny(System::String ^ str, array<wchar_t>^ anyOf, int startIndex);

        static int __clrcall IndexOfAny(System::String ^ str, array<wchar_t>^ anyOf, int startIndex, int count);

#ifdef INTRINSICS_TEST
        // use to make optim and compare results
        static bool __clrcall IndexOfAllWip(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        static bool __clrcall IndexOfAllCli(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        static bool __clrcall IndexOfAllCpp(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        static int __clrcall IndexOfAnyCli(System::String ^ str, array<wchar_t>^ anyOf, int startIndex, int count);

        static int __clrcall IndexOfAnyCpp(System::String ^ str, array<wchar_t>^ anyOf, int startIndex, int count);
#endif
    };
}
