using System;
using System.Text;
using System.Diagnostics;
using Intrinsics;

namespace Intrinsics.Test
{
    public static class StrCs
    {
        public static bool IndexOfAllCs(this string str, string chars, ref Str.MatchIndex[] results, out int resultCount, int startIndex, int count)
        {
            if (chars.Length > Intrinsics.Str.SearchCharsMax)
                throw new ArgumentOutOfRangeException(String.Format("chars length must be smaller than {0}", Intrinsics.Str.SearchCharsMax));

            if ( startIndex < 0 || startIndex + 1 > str.Length)
                throw new ArgumentOutOfRangeException("startIndex must be greater than 0 and smaller than str length - 1");

            if (count > str.Length - startIndex)
                throw new ArgumentOutOfRangeException("count must be smaller than str - startIndex");

            if (results.Length < str.Length)
                results = new Str.MatchIndex[str.Length];

            resultCount = 0;
            int end = startIndex + count;
            for (int i = startIndex; i < end; ++i)
            {
                for (int j = 0; j < chars.Length; ++j)
                {
                    if (str[i] == chars[j])
                    {
                        results[resultCount].StringIndex = i;
                        results[resultCount++].CharIndex = j;
                        break;
                    }
                }
            }
            return resultCount != 0;
        }
    }

    class Program
    {
        enum IndexOfAll
        {
            Sse,
            SseV2,
            Cli,
            Cs,
            Cpp,
            Count
        }

        public class StringTest
        {
            public StringTest(string v)
            {
                value = v;
                for (int i = 0; i < (int)IndexOfAll.Count; ++i)
                    times[i] = new Stopwatch();
            }

            public int call = 0;
            public int match = 0;
            public string value;
            public Stopwatch[] times = new Stopwatch[(int)IndexOfAll.Count];
            public Str.MatchIndex[] results = new Str.MatchIndex[1024 * 16];

            public void Test(string chars)
            {

                for (int i = 0; i < value.Length-1; ++i)
                {
                    Str.MatchIndex[] sseResult = new Str.MatchIndex[value.Length];
                    int sseResultCount;
                    value.IndexOfAll(chars, ref sseResult, out sseResultCount, i, value.Length - i);

                    Str.MatchIndex[] csResult = new Str.MatchIndex[value.Length];
                    int csResultCount;
                    value.IndexOfAllCs(chars, ref csResult, out csResultCount, i, value.Length - i);

                    Str.MatchIndex[] cliResult = new Str.MatchIndex[value.Length];
                    int cliResultCount;
                    value.IndexOfAllCli(chars, ref cliResult, out cliResultCount, i, value.Length - i);

                    Str.MatchIndex[] cppResult = new Str.MatchIndex[value.Length];
                    int cppResultCount;
                    value.IndexOfAllCpp(chars, ref cppResult, out cppResultCount, i, value.Length - i);

                    Str.MatchIndex[] v2Result = new Str.MatchIndex[value.Length];
                    int v2ResultCount;
                    value.IndexOfAllV2(chars, ref v2Result, out v2ResultCount, i, value.Length - i);

                    Debug.Assert(sseResultCount == csResultCount);
                    Debug.Assert(sseResultCount == cliResultCount);
                    Debug.Assert(sseResultCount == cppResultCount);
                    Debug.Assert(sseResultCount == v2ResultCount);

                    for (int j = 0; j < sseResultCount; ++j)
                    {
                        Debug.Assert(sseResult[j].StringIndex == csResult[j].StringIndex);
                        Debug.Assert(sseResult[j].StringIndex == cliResult[j].StringIndex);
                        Debug.Assert(sseResult[j].StringIndex == cppResult[j].StringIndex);
                        Debug.Assert(sseResult[j].StringIndex == v2Result[j].StringIndex);

                        Debug.Assert(sseResult[j].CharIndex == csResult[j].CharIndex);
                        Debug.Assert(sseResult[j].CharIndex == cliResult[j].CharIndex);
                        Debug.Assert(sseResult[j].CharIndex == cppResult[j].CharIndex);
                        Debug.Assert(sseResult[j].CharIndex == v2Result[j].CharIndex);
                    }
                }


                for (int i = value.Length - 1; i >= 0; --i)
                {
                    Str.MatchIndex[] sseResult = new Str.MatchIndex[value.Length];
                    int sseResultCount;
                    value.IndexOfAll(chars, ref sseResult, out sseResultCount, i, value.Length - i);

                    Str.MatchIndex[] csResult = new Str.MatchIndex[value.Length];
                    int csResultCount;
                    value.IndexOfAllCs(chars, ref csResult, out csResultCount, i, value.Length - i);

                    Str.MatchIndex[] cliResult = new Str.MatchIndex[value.Length];
                    int cliResultCount;
                    value.IndexOfAllCli(chars, ref cliResult, out cliResultCount, i, value.Length - i);

                    Str.MatchIndex[] cppResult = new Str.MatchIndex[value.Length];
                    int cppResultCount;
                    value.IndexOfAllCpp(chars, ref cppResult, out cppResultCount, i, value.Length - i);

                    Str.MatchIndex[] v2Result = new Str.MatchIndex[value.Length];
                    int v2ResultCount;
                    value.IndexOfAllV2(chars, ref v2Result, out v2ResultCount, i, value.Length - i);

                    Debug.Assert(sseResultCount == csResultCount);
                    Debug.Assert(sseResultCount == cliResultCount);
                    Debug.Assert(sseResultCount == cppResultCount);
                    Debug.Assert(sseResultCount == v2ResultCount);

                    for (int j = 0; j < sseResultCount; ++j)
                    {
                        Debug.Assert(sseResult[j].StringIndex == csResult[j].StringIndex);
                        Debug.Assert(sseResult[j].StringIndex == cliResult[j].StringIndex);
                        Debug.Assert(sseResult[j].StringIndex == cppResult[j].StringIndex);
                        Debug.Assert(sseResult[j].StringIndex == v2Result[j].StringIndex);

                        Debug.Assert(sseResult[j].CharIndex == csResult[j].CharIndex);
                        Debug.Assert(sseResult[j].CharIndex == cliResult[j].CharIndex);
                        Debug.Assert(sseResult[j].CharIndex == cppResult[j].CharIndex);
                        Debug.Assert(sseResult[j].CharIndex == v2Result[j].CharIndex);
                    }
                }
            }

            public void Profile(string chars)
            {
                call++;

                // warm up cache
                value.IndexOfAllCpp(chars, ref results, out match, 0, value.Length);

                times[(int)IndexOfAll.Sse].Start();
                value.IndexOfAll(chars, ref results, out match, 0, value.Length);
                times[(int)IndexOfAll.Sse].Stop();

                times[(int)IndexOfAll.Cli].Start();
                value.IndexOfAllCli(chars, ref results, out match, 0, value.Length);
                times[(int)IndexOfAll.Cli].Stop();

                times[(int)IndexOfAll.Cs].Start();
                value.IndexOfAllCs(chars, ref results, out match, 0, value.Length);
                times[(int)IndexOfAll.Cs].Stop();

                times[(int)IndexOfAll.Cpp].Start();
                value.IndexOfAllCpp(chars, ref results, out match, 0, value.Length);
                times[(int)IndexOfAll.Cpp].Stop();

                times[(int)IndexOfAll.SseV2].Start();
                value.IndexOfAllV2(chars, ref results, out match, 0, value.Length);
                times[(int)IndexOfAll.SseV2].Stop();
            }
        }

        static void Main(string[] args)
        {
            // create strings

            // perf test
            const string stringChars = "012345679abcdefgzhjklmnopqrstuvwxyz";
            const int stringLengthMin = 8;
            const int stringLengthMax = 1024;
            const int stringLengthIncrement = 64;
            const int stringCount = stringLengthMax / stringLengthIncrement;
            const string searchChars = "[](){}";
            const int stringCharsCount = 16;

            StringTest[] strings = new StringTest[stringCount];
            StringBuilder builder = new StringBuilder();
            Random random = new Random();
            
            int stringLength = stringLengthMin;
            for (int i = 0; i < stringCount; ++i)
            {
                builder.Clear();

                for (int y = 0; y < stringLength; ++y)
                    builder.Append(stringChars[random.Next(0, stringChars.Length)]);

                for (int j = 0; j < stringCharsCount; ++j)
                    builder[random.Next(0, stringLength)] = searchChars[random.Next(0, searchChars.Length)];

                strings[i] = new StringTest(builder.ToString());

                stringLength += stringLengthIncrement;
                if (stringLength > stringLengthMax)
                    stringLength = stringLengthMin;
            }


            for (int i = 0; i < strings.Length; ++i)
            {
                strings[i].Test(searchChars);
            }

            for (int t = 0; t < 512; ++t)
            {
                for (int i = 0; i < strings.Length; ++i)
                {
                    strings[i].Profile(searchChars);
                }
            }

            bool showMs = false;
            if ( showMs )
                System.Console.WriteLine("  call    length  match    cs  ms       cpp ms            v2 ms            sse ms");
            else
                System.Console.WriteLine("  call    length  match    cs ticks     cpp ticks         v2 ticks         sse ticks");
            for (int i = 0; i < strings.Length; ++i)
            {
                StringTest r = strings[i];
                System.Console.WriteLine(
                    "{0,6} {1,6} {2,6}        {3,6}        {4,6}           {5,6}           {6,6}",
                    r.call,
                    r.value.Length,
                    r.match,
                    //showMs ? r.times[(int)IndexOfAll.Cli].ElapsedMilliseconds : r.times[(int)IndexOfAll.Cli].ElapsedTicks,
                    showMs ? r.times[(int)IndexOfAll.Cs].ElapsedMilliseconds : r.times[(int)IndexOfAll.Cs].ElapsedTicks,
                    showMs ? r.times[(int)IndexOfAll.Cpp].ElapsedMilliseconds : r.times[(int)IndexOfAll.Cpp].ElapsedTicks,
                    showMs ? r.times[(int)IndexOfAll.SseV2].ElapsedMilliseconds : r.times[(int)IndexOfAll.SseV2].ElapsedTicks,
                    showMs ? r.times[(int)IndexOfAll.Sse].ElapsedMilliseconds : r.times[(int)IndexOfAll.Sse].ElapsedTicks
                );

                System.Console.WriteLine(
                    "                             {0,5:####0.0}         {1,5:####0.0}            {2,5:####0.0}            {3,5:####0.0}",
                    //(float)((double)r.times[(int)IndexOfAll.Cli].ElapsedTicks / (double)r.times[(int)IndexOfAll.Sse].ElapsedTicks),
                    (float)((double)r.times[(int)IndexOfAll.Cs].ElapsedTicks / (double)r.times[(int)IndexOfAll.Sse].ElapsedTicks),
                    (float)((double)r.times[(int)IndexOfAll.Cpp].ElapsedTicks / (double)r.times[(int)IndexOfAll.Sse].ElapsedTicks),
                    (float)((double)r.times[(int)IndexOfAll.SseV2].ElapsedTicks / (double)r.times[(int)IndexOfAll.Sse].ElapsedTicks),
                    1.0f
                );

            }

            System.Console.ReadKey();

        }
    }
}
