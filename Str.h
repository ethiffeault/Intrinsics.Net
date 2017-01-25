#pragma once

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

namespace Intrinsics
{
    static const int SearchCharsMax = 32;

    [System::Runtime::CompilerServices::ExtensionAttribute]
    public ref class Str abstract sealed
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

        [System::Runtime::CompilerServices::ExtensionAttribute]
        static bool __clrcall IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount);

        [System::Runtime::CompilerServices::ExtensionAttribute]
        static bool __clrcall IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex);

        [System::Runtime::CompilerServices::ExtensionAttribute]
        static bool __clrcall IndexOfAll(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

#ifdef INTRINSICS_TEST
        [System::Runtime::CompilerServices::ExtensionAttribute]
        static bool __clrcall IndexOfAllV2(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        [System::Runtime::CompilerServices::ExtensionAttribute]
        static bool __clrcall IndexOfAllCli(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);

        [System::Runtime::CompilerServices::ExtensionAttribute]
        static bool __clrcall IndexOfAllCpp(System::String ^ str, System::String ^ chars, array<MatchIndex >^% results, [Out] int% resultsCount, int startIndex, int count);
#endif
    };
}
