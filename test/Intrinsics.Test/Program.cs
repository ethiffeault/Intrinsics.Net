﻿using System;
using System.Text;
using System.Diagnostics;

namespace IntrinsicsTest
{
    class Program
    {
        static void Main(string[] args)
        {
            StrIndexOfAllTest test = new StrIndexOfAllTest();
            //test.RunTest();
            test.RunProfile();
            System.Console.ReadKey();
        }
    }
}
