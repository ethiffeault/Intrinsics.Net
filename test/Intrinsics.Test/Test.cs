using System;
using System.Text;
using System.Diagnostics;
using Intrinsics;
using System.Collections.Generic;

namespace Intrinsics.Test
{

    public abstract class Test
    {
        public string Name { get; private set;}

        public Test(string name)
        {
            Name = name;
        }

        public abstract void RunTest();
        public abstract void RunProfile();
        public abstract void OutputProfile(SpreadsheetWriter writer);

        protected void CheckTrue(bool condition)
        {
            if (!condition)
                InternalFailure(1, "CheckTrue failed!");
        }

        protected void CheckTrue(bool condition, string msg, params string[] args)
        {
            if (!condition)
                InternalFailure(1, msg, args);
        }

        private void InternalFailure(int skipFrame, string msg, params string[] args)
        {
            string m = String.Format("{0}: error {1}: {2}", GetCallerFileInfo(skipFrame + 1), Name, String.Format(msg, args));
            Console.WriteLine(m);
            if (System.Diagnostics.Debugger.IsAttached)
                System.Diagnostics.Debug.WriteLine(m);
            System.Diagnostics.Debug.Assert(false, m);
        }

        protected void Failure(string msg, params string[] args)
        {
            InternalFailure(1, msg, args);
        }

        private string GetCallerFileInfo(int skipFrames = 0)
        {
            System.Diagnostics.StackTrace t = new System.Diagnostics.StackTrace(true);
            StackFrame frame = t.GetFrame(1 + skipFrames);
            string fileName = frame.GetFileName();
            int fileLine = frame.GetFileLineNumber();
            string info = String.Format("{0}({1})", fileName, fileLine);
            return info;
        }
    }

    public class SpreadsheetWriter
    {
        private List<List<string>> _rows = new List<List<string>>(64);

        public SpreadsheetWriter()
        { 
        }

        public void Clear()
        {
            _rows.Clear();
        }

        public void Write(int row, int column, string value)
        {
            Expand(row + 1, column + 1);
            _rows[row][column] = value;
        }

        private void Expand(int row, int column)
        {
            // expands rows
            for (int i = _rows.Count; i < row; ++i)
                _rows.Add(new List<string>(64));

            // expands columns
            int columnCount = _rows[0].Count;
            if (columnCount < column)
            {
                for (int i = _rows.Count; i < row; ++i)
                {
                    for (int j = columnCount; j < column; ++j)
                        _rows[i].Add(null);
                }
            }
        }

    }

}
