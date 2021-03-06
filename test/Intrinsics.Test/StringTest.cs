﻿using System;
using System.Text;
using System.Diagnostics;

namespace IntrinsicsTest
{
    public class StringTest : Test
    {
        private const int stringSizeMax = 1024 * 8;
        private const int stringCharsCount  = 0;
        private int[] buckets = { 4, 8, 16, 32, 64, 92, 128, 256, 512, 768, 1024, 2048, 4096, stringSizeMax };
        private const string possiblesChar = "012345679abcdefgzhjklmnopqrstuvwxyz";
        private const string searchChars = "[](){}!@#$%^&*";
        private const int stringsPerBucket = 1024 * 1;
        private string[] strings;
        private Intrinsics.String.MatchIndex[] results = new Intrinsics.String.MatchIndex[stringSizeMax];

        private enum IndexOfAll
        {
            Sse,
            SseV2,
            Cli,
            Cs,
            Cpp,
            Count
        }

        private enum IndexOfAny
        {
            Sse,
            Cli,
            Cs,
            Cpp,
            Count
        }


        public StringTest()
            : base("String")
        {
            strings = new string[stringsPerBucket * buckets.Length];

            Random random = new Random();
            StringBuilder builder = new StringBuilder(stringSizeMax);
            int stringLengthMin = 0;
            for (int bucketIndex = 0; bucketIndex < buckets.Length; ++bucketIndex)
            {
                int stringLengthMax = buckets[bucketIndex];

                int stringLength = stringLengthMin;
                for (int i = 0; i < stringsPerBucket; ++i)
                {
                    builder.Clear();
                    for (int c = 0; c < stringLength; ++c)
                        builder.Append(possiblesChar[random.Next(0, possiblesChar.Length)]);

                    if (stringLength != 0)
                    {
                        for (int j = 0; j < stringCharsCount; ++j)
                            builder[random.Next(0, stringLength)] = searchChars[random.Next(0, searchChars.Length)];
                    }

                    strings[bucketIndex * stringsPerBucket + i] = builder.ToString();

                    if (++stringLength >= stringLengthMax)
                        stringLength = stringLengthMin;
                }

                stringLengthMin = stringLengthMax;
            }
        }

        public override void RunTest()
        {
            for (int i = 0; i < strings.Length; ++i)
            {
                string s = strings[i];
                TestIndexOfAll(s, searchChars, 0, s.Length);

                if (i == (strings.Length / 2))
                {
                    for (int startIndex = 0; startIndex < s.Length - 1; ++startIndex)
                    {
                        int count = s.Length - startIndex;
                        TestIndexOfAll(s, searchChars, startIndex, count);
                        TestIndexOfAll(s, searchChars, 0, startIndex + 1);

                        TestIndexOfAny(s, searchChars, startIndex, count);
                        TestIndexOfAny(s, searchChars, 0, startIndex + 1);

                    }
                }
            }
        }

        public override void RunProfile()
        {
            bool runIndexOfAll = false;
            bool runIndexOfAny = true;

            if ( runIndexOfAll)
            {
                System.Console.WriteLine("IndexOfAll");

                Stopwatch[] times = new Stopwatch[(int)IndexOfAll.Count];
                for (int i = 0; i < times.Length; ++i)
                    times[i] = new Stopwatch();

                bool outputHeader = true;

                for (int bucketIndex = 0; bucketIndex < buckets.Length; ++bucketIndex)
                {
                    for (int i = 0; i < stringsPerBucket; ++i)
                    {
                        int stringIndex = bucketIndex * stringsPerBucket + i;

                        string s = strings[stringIndex];
                        int resultsCount;
                        bool haveMatch;

                        // cache warmup size
                        int warmupSize = s.Length == 0 ? 0 : 1;

                        haveMatch = Intrinsics.String.IndexOfAll(s, searchChars, ref results, out resultsCount, 0, warmupSize);
                        times[(int)IndexOfAll.Sse].Start();
                        haveMatch = Intrinsics.String.IndexOfAll(s, searchChars, ref results, out resultsCount, 0, s.Length);
                        times[(int)IndexOfAll.Sse].Stop();

                        haveMatch = Intrinsics.String.IndexOfAllCli(s, searchChars, ref results, out resultsCount, 0, warmupSize);
                        times[(int)IndexOfAll.Cli].Start();
                        haveMatch = Intrinsics.String.IndexOfAllCli(s, searchChars, ref results, out resultsCount, 0, s.Length);
                        times[(int)IndexOfAll.Cli].Stop();

                        haveMatch = StringCs.IndexOfAll(s, searchChars, ref results, out resultsCount, 0, warmupSize);
                        times[(int)IndexOfAll.Cs].Start();
                        haveMatch = StringCs.IndexOfAll(s, searchChars, ref results, out resultsCount, 0, s.Length);
                        times[(int)IndexOfAll.Cs].Stop();

                        haveMatch = Intrinsics.String.IndexOfAllCpp(s, searchChars, ref results, out resultsCount, 0, warmupSize);
                        times[(int)IndexOfAll.Cpp].Start();
                        haveMatch = Intrinsics.String.IndexOfAllCpp(s, searchChars, ref results, out resultsCount, 0, s.Length);
                        times[(int)IndexOfAll.Cpp].Stop();

                        haveMatch = Intrinsics.String.IndexOfAllWip(s, searchChars, ref results, out resultsCount, 0, warmupSize);
                        times[(int)IndexOfAll.SseV2].Start();
                        haveMatch = Intrinsics.String.IndexOfAllWip(s, searchChars, ref results, out resultsCount, 0, s.Length);
                        times[(int)IndexOfAll.SseV2].Stop();
                    }

                    // write bucket stats!
                    if (outputHeader)
                    {
                        System.Console.WriteLine("length        sse          cs         cpp         cli         wip");
                        outputHeader = false;
                    }

                    System.Console.WriteLine(
                        "{0,6}      {1,5:###0.00}       {2,5:###0.00}       {3,5:###0.00}       {4,5:###0.00}       {5,5:###0.00}",
                        buckets[bucketIndex],
                        1.0f,
                        (float)((double)times[(int)IndexOfAll.Cs].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks),
                        (float)((double)times[(int)IndexOfAll.Cpp].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks),
                        (float)((double)times[(int)IndexOfAll.Cli].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks),
                        (float)((double)times[(int)IndexOfAll.SseV2].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks)
                    );
                }
            }

            if (runIndexOfAny)
            {
                System.Console.WriteLine("IndexOfAny");

                Stopwatch[] times = new Stopwatch[(int)IndexOfAny.Count];
                for (int i = 0; i < times.Length; ++i)
                    times[i] = new Stopwatch();

                bool outputHeader = true;

                char[] chars = searchChars.ToCharArray();

                for (int bucketIndex = 0; bucketIndex < buckets.Length; ++bucketIndex)
                {
                    for (int i = 0; i < stringsPerBucket; ++i)
                    {
                        int stringIndex = bucketIndex * stringsPerBucket + i;

                        string s = strings[stringIndex];
                        int resultsCount;

                        // cache warmup size
                        int warmupSize = s.Length == 0 ? 0 : 1;

                        resultsCount = Intrinsics.String.IndexOfAny(s, chars, 0, warmupSize);
                        times[(int)IndexOfAny.Sse].Start();
                        resultsCount = Intrinsics.String.IndexOfAny(s, chars, 0, s.Length);
                        times[(int)IndexOfAny.Sse].Stop();

                        resultsCount = Intrinsics.String.IndexOfAnyCli(s, chars, 0, warmupSize);
                        times[(int)IndexOfAny.Cli].Start();
                        resultsCount = Intrinsics.String.IndexOfAnyCli(s, chars, 0, s.Length);
                        times[(int)IndexOfAny.Cli].Stop();

                        resultsCount = s.IndexOfAny(chars, 0, warmupSize);
                        times[(int)IndexOfAny.Cs].Start();
                        resultsCount = s.IndexOfAny(chars, 0, s.Length);
                        times[(int)IndexOfAny.Cs].Stop();

                        resultsCount = Intrinsics.String.IndexOfAnyCpp(s, chars, 0, warmupSize);
                        times[(int)IndexOfAny.Cpp].Start();
                        resultsCount = Intrinsics.String.IndexOfAnyCpp(s, chars, 0, s.Length);
                        times[(int)IndexOfAny.Cpp].Stop();
                    }

                    // write bucket stats!
                    if (outputHeader)
                    {
                        System.Console.WriteLine("length        sse          cs         cpp         cli");
                        outputHeader = false;
                    }

                    System.Console.WriteLine(
                        "{0,6}      {1,5:###0.00}       {2,5:###0.00}       {3,5:###0.00}       {4,5:###0.00}",
                        buckets[bucketIndex],
                        1.0f,
                        (float)((double)times[(int)IndexOfAny.Cs].ElapsedTicks / (double)times[(int)IndexOfAny.Sse].ElapsedTicks),
                        (float)((double)times[(int)IndexOfAny.Cpp].ElapsedTicks / (double)times[(int)IndexOfAny.Sse].ElapsedTicks),
                        (float)((double)times[(int)IndexOfAny.Cli].ElapsedTicks / (double)times[(int)IndexOfAny.Sse].ElapsedTicks)
                    );
                }
            }

        }

        public override void OutputProfile(SpreadsheetWriter writer)
        {
        }

        private void TestIndexOfAll(string s, string chars, int startIndex, int count)
        {
            Intrinsics.String.MatchIndex[] sseResult = new Intrinsics.String.MatchIndex[s.Length];
            int sseResultCount;
            Intrinsics.String.IndexOfAll(s, chars, ref sseResult, out sseResultCount, startIndex, count);

            Intrinsics.String.MatchIndex[] csResult = new Intrinsics.String.MatchIndex[s.Length];
            int csResultCount;
            StringCs.IndexOfAll(s, chars, ref csResult, out csResultCount, startIndex, count);

            Intrinsics.String.MatchIndex[] cliResult = new Intrinsics.String.MatchIndex[s.Length];
            int cliResultCount;
            Intrinsics.String.IndexOfAllCli(s, chars, ref cliResult, out cliResultCount, startIndex, count);

            Intrinsics.String.MatchIndex[] v2Result = new Intrinsics.String.MatchIndex[s.Length];
            int v2ResultCount;
            Intrinsics.String.IndexOfAllWip(s, chars, ref v2Result, out v2ResultCount, startIndex, count);

            Intrinsics.String.MatchIndex[] cppResult = new Intrinsics.String.MatchIndex[s.Length];
            int cppResultCount;
            Intrinsics.String.IndexOfAllCpp(s, chars, ref cppResult, out cppResultCount, startIndex, count);

            CheckTrue(sseResultCount == csResultCount);
            CheckTrue(sseResultCount == cliResultCount);
            CheckTrue(sseResultCount == cppResultCount);
            CheckTrue(sseResultCount == v2ResultCount);

            for (int j = 0; j < sseResultCount; ++j)
            {
                CheckTrue(sseResult[j].StringIndex == csResult[j].StringIndex);
                CheckTrue(sseResult[j].StringIndex == cliResult[j].StringIndex);
                CheckTrue(sseResult[j].StringIndex == cppResult[j].StringIndex);
                CheckTrue(sseResult[j].StringIndex == v2Result[j].StringIndex);

                CheckTrue(chars[sseResult[j].CharIndex] == chars[csResult[j].CharIndex]);
                CheckTrue(chars[sseResult[j].CharIndex] == chars[cliResult[j].CharIndex]);
                CheckTrue(chars[sseResult[j].CharIndex] == chars[cppResult[j].CharIndex]);
                CheckTrue(chars[sseResult[j].CharIndex] == chars[v2Result[j].CharIndex]);
            }
        }

        private void TestIndexOfAny(string s, string charsString, int startIndex, int count)
        {
            char[] chars = charsString.ToCharArray();

            int sseResultCount = Intrinsics.String.IndexOfAny(s, chars, startIndex, count);
            int csResultCount = s.IndexOfAny(chars, startIndex, count);
            int cliResultCount = Intrinsics.String.IndexOfAny(s, chars, startIndex, count);
            int cppResultCount = Intrinsics.String.IndexOfAny(s, chars, startIndex, count);

            CheckTrue(sseResultCount == csResultCount);
            CheckTrue(sseResultCount == cliResultCount);
            CheckTrue(sseResultCount == cppResultCount);
        }
    }

    public static class StringCs
    {
        public static bool IndexOfAll(string str, string chars, ref Intrinsics.String.MatchIndex[] results, out int resultCount, int startIndex, int count)
        {
            if (chars.Length > Intrinsics.String.SearchCharsMax)
                throw new ArgumentOutOfRangeException(String.Format("chars length must be smaller than {0}", Intrinsics.String.SearchCharsMax));

            if (count == 0)
            {
                resultCount = 0;
                return false;
            }

            if (startIndex < 0 || startIndex + 1 > str.Length)
                throw new ArgumentOutOfRangeException("startIndex must be greater than 0 and smaller than str length - 1");

            if (count > str.Length - startIndex)
                throw new ArgumentOutOfRangeException("count must be smaller than str - startIndex");

            if (results.Length < str.Length)
                results = new Intrinsics.String.MatchIndex[str.Length];

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
}