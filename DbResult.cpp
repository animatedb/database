/*
* DbResult.h
*
*  Created: 2016
*  \copyright 2016 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
#include "DbResult.h"
#include <map>
#include <mutex>
#ifndef __linux__
#include <conio.h>
#endif


class DbResultContext
    {
    public:
        DbResultContext():
            mResultIndex(DbResult::RES_START)
            {}
        ~DbResultContext()
            {
            std::lock_guard<std::mutex> lock(mMutex);
            if(mErrorStrings.size() != 0)
                {
                fprintf(stderr, "Unhandled errors\n");
                for(auto const &err:mErrorStrings)
                    {
                    fprintf(stderr, "  %s\n", err.second.c_str());
                    }
                // This should not be done because it will hang processes, where the stderr text
                // may not be visible.
//                _getch();
                }
            }

        int setErrorOrWarning(std::string const &errStr)
            {
            int resultId = mResultIndex;
            std::lock_guard<std::mutex> lock(mMutex);
            mErrorStrings[mResultIndex] = errStr;
            // Increment with rollover.
            mResultIndex = (mResultIndex & DbResult::RES_CODEMASK) + 1;
            return resultId;
            }

        void insertContext(int resultId, std::string const &errStr)
            {
            std::lock_guard<std::mutex> lock(mMutex);
            std::string str = errStr;
            str += "\n";
            str += mErrorStrings[resultId & DbResult::RES_CODEMASK];
            mErrorStrings[resultId & DbResult::RES_CODEMASK] = str;
            }

        std::string const getErrorString(int resultId)
            {
            std::lock_guard<std::mutex> lock(mMutex);
            auto iter = mErrorStrings.find(resultId & DbResult::RES_CODEMASK);
            std::string errStr;
            if(iter != mErrorStrings.end())
                {
                errStr = iter->second;
                mErrorStrings.erase(iter);
                }
            return(errStr);
            };
/*
        std::string const getAllErrors()
            {
            std::string errStr;
            for(auto const &err : mErrorStrings)
                {
                if(errStr.length() == 0)
                    {
                    errStr += ", ";
                    }
                errStr += err.second.c_str();
                }
            return errStr;
            }
*/

    private:
        int mResultIndex;
        std::mutex mMutex;
        std::map<int, std::string> mErrorStrings;
    };

DbResultContext gDbResultContext;

void DbResult::setError(std::string const &errStr)
    {
    if(!(mResultId & RES_ERROR))
        {
        // This should probably remove the warning string since the warning
        // is being overwritten. See comments for getMrError().
        mResultId = gDbResultContext.setErrorOrWarning(errStr);
        mResultId |= RES_ERROR;
        }
    else
        {
        std::string addedErrStr = "Multiple errors: ";
        addedErrStr += errStr;
        gDbResultContext.insertContext(mResultId, addedErrStr);
        }
    }

void DbResult::setWarning(std::string const &errStr)
    {
    // Don't overwrite error codes.
    if(!(mResultId & RES_ERROR))
        {
        mResultId = gDbResultContext.setErrorOrWarning(errStr);
        mResultId |= RES_WARNING;
        }
    }

void DbResult::insertContext(std::string const &errStr)
    {
    if(!(mResultId & RES_ERROR))
        {
        setError("Setting context before error");
        }
    gDbResultContext.insertContext(mResultId, errStr);
    }

std::string const getDbResultString(DbResult const &result)
    {
    return gDbResultContext.getErrorString(result.getResultId());
    }

