using System;
using System.Text;
using System.Diagnostics;

namespace IntrinsicsTest
{
    class Program
    {
        static void Main(string[] args)
        {
            StringTest test = new StringTest();
            //test.RunTest();
            test.RunProfile();
            System.Console.ReadKey();
        }
    }
}
