/*
* DbResult.h
*
*  Created: 2017
*  \copyright 2017 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/

#ifndef DB_RESULT_H
#define DB_RESULT_H

#include <string>

// This error class is meant to be as small and fast as a simple integer or boolean in
// the case that there are no errors. When there are errors, it will be slower.
// This class is thread safe as long as billions of errors are not created.
// Errors from all threads get registered to a single set of memory, that can
// be accessed by the error ID in each thread. The error ID's are incremented
// each time there is an error.
//
// - Allows adding high level context information along with detailed errors.
//      DbResult func1()
//          { DbResult result; result.setError("File sharing violation"); return result; }
//      DbResult func2()
//          {   DbResult result = func1();
//              if(!result.isOk())
//                  { result.insertContext("Unable to process results"); }
//              return result; }
//      Displays the following:
//          "Unable to save results. File sharing violation."
//
// - Prevents uninitialized results.
//      int func1()
//          { int errCode; return errCode; }    // Returned error is uninitialized.
//
// - Prevents dropped error return codes from non-returned values.
//      int func1()
//          { int errCode = FAIL; return errCode; }   // Error found here.
//      int func2()
//          { int errCode = OK; func(); return errCode; }    // Return from func1 dropped.
//
// - Prevents dropped error return codes due to multiple error code variables.
//      int func1()
//          { int errCode = FAIL; return errCode; }   // Error found here.
//      int func2(int x)            // Return from func1 dropped because of multiple errCodes.
//          { int errCode = OK; if(x){ int errCode = func(); } return errCode; }
//
// - Prevents dropped errors due to overwrite of an error code variable.
//      Example:
//          int func1()
//              { int errCode = FAIL; return errCode; }
//          int func2()
//              { int errCode = OK; return errCode; }
//          int func3()                             // func2 overwrites errCode from func1.
//              { int errCode = func1(); errCode = func2(); return errCode; }
class DbResult
    {
    friend class DbResultContext;
    public:
        DbResult():
            mResultId(RES_START)
            {}

        /// Returns ok even if a warning was set.
        bool isOk() const
            { return(!(mResultId & RES_ERROR)); }
        bool haveWarning() const
            { return((mResultId & RES_WARNING) > 0); }

        /// Do not put delimiters such as periods in the error strings. They may
        /// be added later.
        void setError(std::string const &errStr);

        /// Do not put delimiters such as periods in the error strings. They may
        /// be added later.
        void insertContext(std::string const &errStr);
        void setWarning(std::string const &errStr);

        /// This code is not useful for detecting the type of error.
        int getResultId() const
            { return mResultId; }

    private:
        /// This stores the result code. Only one code is supported in each DbResult class.
        /// If multiple threads are used, separate DbResults may exist, and may have
        /// different codes. 
        /// In each DbResult
        ///      - Once an error is set, a warning will not be registered.
        ///      - If a warning is set, an error will overwrite it.
        int mResultId;
        static const int RES_START = 0;
        static const int RES_ERROR = 0x80000000;
        static const int RES_WARNING = 0x40000000;
        static const int RES_FLAG_MASK = RES_ERROR | RES_WARNING;
        static const int RES_CODEMASK = ~RES_FLAG_MASK;
    };

/// Returns error or warning strings.
/// This clears the error from the central storage of errors.
/// This has a flaw where if a warning was overwritten with an error, this
/// will clear the error, but not the warning string. See SetError for comments.
std::string const getDbResultString(DbResult const &result);

#endif

