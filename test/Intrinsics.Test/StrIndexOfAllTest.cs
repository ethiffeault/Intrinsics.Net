using System;
using System.Text;
using System.Diagnostics;
using Intrinsics;

namespace Intrinsics.Test
{

    public class StrIndexOfAllTest : Test
    {
        const string possiblesChar = "012345679abcdefgzhjklmnopqrstuvwxyz";
        const string searchChars = "[](){}";
        private const int stringSizeMin = 0;
        private const int stringSizeMax = 1024;
        private const int stringsCount = 1024;
        private const int stringCharsCount = 4;
        private string[] strings = new string[stringsCount];
        private Str.MatchIndex[] results = new Str.MatchIndex[stringSizeMax];

        public StrIndexOfAllTest()
            : base("IndexOfAll")
        {
            int stringSizeIncrement = (stringSizeMax - stringSizeMin) / stringsCount;

            Random random = new Random();
            StringBuilder builder = new StringBuilder(stringSizeMax);

            int stringSize = stringSizeMin;
            for (int i = 0; i < stringsCount; ++i)
            {
                builder.Clear();

                for (int c = 0; c < stringSize; ++c)
                    builder.Append(possiblesChar[random.Next(0, possiblesChar.Length)]);

                if (stringSize != 0)
                {
                    for (int j = 0; j < stringCharsCount; ++j)
                        builder[random.Next(0, stringSize)] = searchChars[random.Next(0, searchChars.Length)];
                }

                strings[i] = builder.ToString();

                if (i == stringsCount - 2)
                    stringSize = stringSizeMax;
                else
                    stringSize += stringSizeIncrement;
            }
        }

        public override void RunTest()
        {
            for (int i = 0; i < stringsCount; ++i)
            {
                string s = strings[i];
                TestIndexOfAll(s, searchChars, 0, s.Length);

                if (i == (stringsCount/2))
                {
                    for (int startIndex = 0; startIndex < s.Length - 1; ++startIndex)
                    {
                        int count = s.Length - startIndex;
                        TestIndexOfAll(s, searchChars, startIndex, count);
                        TestIndexOfAll(s, searchChars, 0, startIndex + 1);
                    }
                }
            }
        }

        enum IndexOfAll
        {
            Sse,
            SseV2,
            Cli,
            Cs,
            Cpp,
            Count
        }

        public override void RunProfile()
        {
            int[] buckets = { 4, 8, 16, 32, 64, 92, 128, 256, 512, 768, stringSizeMax };
            Stopwatch[] times = new Stopwatch[(int)IndexOfAll.Count];
            for (int i = 0; i < times.Length; ++i)
                times[i] = new Stopwatch();

            string[] searchPatterns = { "@#$%" };
            //string[] searchPatterns = { searchChars, " ", "!", searchChars.Substring(0, 1)};
            bool outputHeader = true;
            int stringIndex = 0;
            for (int bucketIndex = 0; bucketIndex < buckets.Length; ++bucketIndex)
            {
                for (int i = 0; i < times.Length; ++i)
                    times[i].Reset();

                // buck run
                int bucketRun = 1024 * 32;
                int currentBucketRun = 0;

                while (currentBucketRun < bucketRun)
                {
                    if (bucketIndex == 0)
                        stringIndex = 0;
                    else
                        stringIndex = buckets[bucketIndex-1];

                    for (int searchIndex = 0; searchIndex < searchPatterns.Length && currentBucketRun < bucketRun; ++searchIndex)
                    {
                        for (; stringIndex < buckets[bucketIndex] && currentBucketRun < bucketRun; ++stringIndex, ++currentBucketRun)
                        {
                            string s = strings[stringIndex];
                            int resultsCount;
                            bool haveMatch;

                            // cache warmup size
                            int warmupSize = s.Length == 0 ? 0 : 1;

                            haveMatch = s.IndexOfAll(searchChars, ref results, out resultsCount, 0, warmupSize);
                            times[(int)IndexOfAll.Sse].Start();
                            haveMatch = s.IndexOfAll(searchChars, ref results, out resultsCount, 0, s.Length);
                            times[(int)IndexOfAll.Sse].Stop();

                            haveMatch = s.IndexOfAllCli(searchChars, ref results, out resultsCount, 0, warmupSize);
                            times[(int)IndexOfAll.Cli].Start();
                            haveMatch = s.IndexOfAllCli(searchChars, ref results, out resultsCount, 0, s.Length);
                            times[(int)IndexOfAll.Cli].Stop();

                            haveMatch = s.IndexOfAllCs(searchChars, ref results, out resultsCount, 0, warmupSize);
                            times[(int)IndexOfAll.Cs].Start();
                            haveMatch = s.IndexOfAllCs(searchChars, ref results, out resultsCount, 0, s.Length);
                            times[(int)IndexOfAll.Cs].Stop();

                            haveMatch = s.IndexOfAllCpp(searchChars, ref results, out resultsCount, 0, warmupSize);
                            times[(int)IndexOfAll.Cpp].Start();
                            haveMatch = s.IndexOfAllCpp(searchChars, ref results, out resultsCount, 0, s.Length);
                            times[(int)IndexOfAll.Cpp].Stop();

                            haveMatch = s.IndexOfAllV2(searchChars, ref results, out resultsCount, 0, warmupSize);
                            times[(int)IndexOfAll.SseV2].Start();
                            haveMatch = s.IndexOfAllV2(searchChars, ref results, out resultsCount, 0, s.Length);
                            times[(int)IndexOfAll.SseV2].Stop();
                        }
                    }
                }

                // write bucket stats!
                if (outputHeader)
                {
                    System.Console.WriteLine("length        cli          cs         cpp          v2         sse");
                    outputHeader = false;
                }

                System.Console.WriteLine(
                    "{0,6}      {1,5:####0.0}       {2,5:####0.0}       {3,5:####0.0}       {4,5:####0.0}       {5,5:####0.0}",
                    buckets[bucketIndex],
                    (float)((double)times[(int)IndexOfAll.Cli].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks),
                    (float)((double)times[(int)IndexOfAll.Cs].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks),
                    (float)((double)times[(int)IndexOfAll.Cpp].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks),
                    (float)((double)times[(int)IndexOfAll.SseV2].ElapsedTicks / (double)times[(int)IndexOfAll.Sse].ElapsedTicks),
                    1.0f
                );


            }


            


        }

        public override void OutputProfile(SpreadsheetWriter writer)
        {
        }

        private void TestIndexOfAll(string value, string chars, int startIndex, int count)
        {
            Str.MatchIndex[] sseResult = new Str.MatchIndex[value.Length];
            int sseResultCount;
            value.IndexOfAll(chars, ref sseResult, out sseResultCount, startIndex, count);

            Str.MatchIndex[] csResult = new Str.MatchIndex[value.Length];
            int csResultCount;
            value.IndexOfAllCs(chars, ref csResult, out csResultCount, startIndex, count);

            Str.MatchIndex[] cliResult = new Str.MatchIndex[value.Length];
            int cliResultCount;
            value.IndexOfAllCli(chars, ref cliResult, out cliResultCount, startIndex, count);

            Str.MatchIndex[] v2Result = new Str.MatchIndex[value.Length];
            int v2ResultCount;
            value.IndexOfAllV2(chars, ref v2Result, out v2ResultCount, startIndex, count);

            Str.MatchIndex[] cppResult = new Str.MatchIndex[value.Length];
            int cppResultCount;
            value.IndexOfAllCpp(chars, ref cppResult, out cppResultCount, startIndex, count);

            if (sseResultCount != cppResultCount)
                value.IndexOfAllCpp(chars, ref cppResult, out cppResultCount, startIndex, count);


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

                CheckTrue(sseResult[j].CharIndex == csResult[j].CharIndex);
                CheckTrue(sseResult[j].CharIndex == cliResult[j].CharIndex);
                CheckTrue(sseResult[j].CharIndex == cppResult[j].CharIndex);
                CheckTrue(sseResult[j].CharIndex == v2Result[j].CharIndex);
            }            
        }
    }

    public static class StrIndexOfAllTestCs
    {
        public static bool IndexOfAllCs(this string str, string chars, ref Str.MatchIndex[] results, out int resultCount, int startIndex, int count)
        {
            if (chars.Length > Intrinsics.Str.SearchCharsMax)
                throw new ArgumentOutOfRangeException(String.Format("chars length must be smaller than {0}", Intrinsics.Str.SearchCharsMax));

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

}