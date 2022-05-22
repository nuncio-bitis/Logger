/*
 * This file is part of the Logger distribution
 *   (https://github.com/nuncio-bitis/Logger
 * Copyright (c) 2022 James P. Parziale.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Logger.cpp
 *
 *  Created on: Aug 10, 2011
 *      Author: J. Parziale
 *
 *  History:
 *  00 08/10/2011
 *    Original version.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>

#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>

#include "Logger.h"

//******************************************************************************

Logger *Logger::s_pInstance = nullptr;

std::ofstream *Logger::m_logFile;
std::string Logger::m_logFileName;
std::string Logger::mFileNameBase;
Severity Logger::m_leastSeverity;
std::mutex Logger::m_mutex;
std::stringstream Logger::m_outString;
unsigned int Logger::mMaxFileSize = DEFAULT_MAX_FILE_SIZE;
unsigned int Logger::mMaxFileNumber = DEFAULT_MAX_FILE_NUMBER;
std::string Logger::mHeader;
bool Logger::mBuffering;
std::vector<std::string> Logger::mLogFileList;
std::vector<std::string> Logger::mBufferList;
bool Logger::mUsingStdOut = false;

/**
 * String versions of severity levels.
 */
std::map<Severity, std::string> Logger::s_severityString = {
    {eLOG_CRIT,  "CRIT_"},
    {eLOG_HIGH,  "HIGH_"},
    {eLOG_MED,   "MED__"},
    {eLOG_LOW,   "LOW__"},
    {eLOG_INFO,  "INFO_"},
    {eLOG_DEBUG, "DEBUG"},
};

//******************************************************************************

/**
 * Constructor used such that all log output goes to console.
 */
Logger::Logger(Severity severity)
{
    s_pInstance = this;

    m_leastSeverity = severity;
    m_logFileName = "<STDOUT>";
    mUsingStdOut = true;
    m_logFile = nullptr;
    mBuffering = false;

    std::cerr << "Logging at log level "
              << severityToString(m_leastSeverity)
              << " to "
              << m_logFileName
              << std::endl;

    m_outString << "--------------------------------------------------------------------------------" << std::endl;
    m_outString << "START: " + TimeStamp() << std::endl;
    m_outString << " filename = " << m_logFileName << std::endl;
    m_outString << " severity = " << severityToString(m_leastSeverity) << std::endl;
    m_outString << "--------------------------------------------------------------------------------" << std::endl;
    writeLog(m_outString);
}

//******************************************************************************

/**
 * Constructor used such that all log output goes to set of log files.
 * Mirroring output to console may be enabled using MirrorToStdOut()
 */
Logger::Logger(const std::string baseName, Severity severity)
{
    s_pInstance = this;

    m_leastSeverity = severity;
    mFileNameBase = baseName;
    mUsingStdOut = false;
    mBuffering = true;

    MakeFileList();
    GenerateFileName();

    if (Open(std::ios::app | std::ios::out))
    {
        EnforceLogFileLimits();
    }
    else
    {
        std::cerr << "Error: unable to open output file: " << m_logFileName << std::endl;
        return;
    }

    std::cerr << "Successfully opened output log file at log level "
              << severityToString(m_leastSeverity)
              << ", name = "
              << m_logFileName
              << std::endl;

    m_outString << "--------------------------------------------------------------------------------" << std::endl;
    m_outString << "START: " + TimeStamp() << std::endl;
    m_outString << " filename = " << m_logFileName << std::endl;
    m_outString << " severity = " << severityToString(m_leastSeverity) << std::endl;
    m_outString << "--------------------------------------------------------------------------------" << std::endl;
    writeLog(m_outString);
}

//******************************************************************************

Logger::~Logger()
{
    if (s_pInstance != nullptr)
    {
        m_outString << "--------------------------------------------------------------------------------" << std::endl;
        m_outString << "FINISH: " + TimeStamp() << std::endl;
        m_outString << "--------------------------------------------------------------------------------" << std::endl;
        writeLog(m_outString);

        // Make sure to flush internal buffer to file.
        WriteToFile();

        if (m_logFile != nullptr)
        {
            m_logFile->close();
        }

        delete m_logFile;
        s_pInstance = nullptr;
    }
}

//******************************************************************************

Logger *Logger::getInstance()
{
    return s_pInstance;
}

//******************************************************************************

bool Logger::setSeverity(Severity s)
{
    if ((s < eLOG_CRIT) || (s > eLOG_LAST))
    {
        m_outString << TimeStamp() << " ERROR: Invalid verbosity specified, " << s << std::endl;
        writeLog(m_outString);
        return false;
    }

    auto locked = std::unique_lock<std::mutex>(m_mutex);
    m_outString << TimeStamp() << " Set severity level to " << s_severityString[s] << std::endl;
    m_leastSeverity = s;
    writeLog(m_outString);

    return true;
}

//******************************************************************************

void Logger::log(Severity s, const std::string &input)
{
    auto locked = std::unique_lock<std::mutex>(m_mutex);
    if (s <= m_leastSeverity)
    {
        m_outString << TimeStamp() << " [" << severityToString(s) << "] " << input << std::endl;
        writeLog(m_outString);
    }
}

//******************************************************************************

void Logger::log(Severity s, std::stringstream &input)
{
    auto locked = std::unique_lock<std::mutex>(m_mutex);
    if (s <= m_leastSeverity)
    {
        m_outString << TimeStamp() << " [" << severityToString(s) << "] " << input.str() << std::endl;
        writeLog(m_outString);
    }

    // Empty the stringstream after use
    input.str(std::string());
    input.clear();
}

//******************************************************************************

void Logger::log(Severity s, const char *logline, ...)
{
    auto locked = std::unique_lock<std::mutex>(m_mutex);
    if (s <= m_leastSeverity)
    {
        va_list argList;
        char cbuffer[1024];
        va_start(argList, logline);
        vsnprintf(cbuffer, 1024, logline, argList);
        va_end(argList);
        m_outString << TimeStamp() << " [" << severityToString(s) << "] " << cbuffer << std::endl;
        writeLog(m_outString);
    }
}

//*****************************************************************************

/*! This function enables and disables buffering.  If going from enabled to
 *  disabled, the current contents of the buffer are written.  If going from
 *  disabled to enabled, the buffer is cleared.
 *
 *  \param on If true, enables buffering.
 */
void Logger::SetBuffering(bool on)
{
    if (mBuffering && !on)
    {
        WriteToFile();
    }
    else if (!mBuffering && on)
    {
        mBufferList.clear();
    }
    mBuffering = on;
};

//*****************************************************************************

/*! Flush buffer to disk
 */
void Logger::WriteToFile()
{
    while (mBufferList.size())
    {
        m_outString << Get();
        writeLog(m_outString);
    }
    Flush();
}

//*****************************************************************************

bool Logger::Open(std::ios::openmode mode)
{
    // no file name yet
    if (m_logFileName.empty())
    {
        return false;
    }

    mode |= std::ios::binary;

    // Get the current directory
    char cwd[1024];
    std::string currentDir;
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        currentDir = cwd;
    }
    else
    {
        perror("getcwd() error");
        return false;
    }

    int pathIdx = m_logFileName.find_last_of('/');
    if (pathIdx < 0)
    {
        std::string full = currentDir + "/" + m_logFileName;
        m_logFileName = full;
    }

    m_logFile = new std::ofstream(m_logFileName, mode);

    return (m_logFile != nullptr);
}

//*****************************************************************************

/*! based on mFileBaseName, find all corresponding files
 *
 *  \return true if the list is made, false otherwise.
 */
bool Logger::MakeFileList()
{
    // empty base name
    if (mFileNameBase.empty())
    {
        return false;
    }

    std::string logDir;
    std::string workingDir;

    // save the current directory
    char cwd[132];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        workingDir = cwd;
    }
    else
    {
        perror("getcwd() error");
        return false;
    }

    // extract the path
    int idx = mFileNameBase.find_last_of('/');

    // use the path
    if (idx >= 0)
    {
        // absolute path
        if (mFileNameBase[0] == '/')
        {
            logDir = mFileNameBase.substr(0, idx);
        }
        else
        {
            logDir = workingDir + mFileNameBase.substr(0, idx);
        }
    }
    else
    {
        logDir = workingDir;
    }
    (void)chdir(logDir.c_str());

    // empty the list
    mLogFileList.clear();

    // Need to only add correctly formatted names, but for now only compare the size
    const std::size_t nameLength = mFileNameBase.substr(idx + 1).size() + std::string("_yyyy-MM-dd").size() + std::string("_HHmmss").size();
    const std::string filenamebase = mFileNameBase.substr(idx + 1); // file basename (everything after path)

    // Parse all the files and directories within this directory matching "<mFileNameBase>_yyyy-MM-dd_HHmmss"
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(logDir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {

            std::string entry(ent->d_name);

            // Add file matching "<mFileNameBase>_yyyy-MM-dd_HHmmss"
            if ((entry.size() == nameLength) && (entry.substr(filenamebase.size()) == filenamebase))
            {
                mLogFileList.push_back(logDir + "/" + entry);
            }
        }
        closedir(dir);
    }
    else
    {
        /* could not open directory */
        perror("");
        return EXIT_FAILURE;
    }

    // return to the original directory
    (void)chdir(workingDir.c_str());

    return true;
}

//*****************************************************************************

/*! Create New File name, set as the current file, and add into the list
 */
void Logger::AddNewFileName()
{
    std::string name(mFileNameBase + "_" + GetDateStr(false) + "_" + GetTimeStr(false, false) + ".log");
    mLogFileList.push_back(name);
    m_logFileName = name;
}

//*****************************************************************************

/*! To set the file name, first get the last file name
 *  and check the size create new one file if necessary.
 *
 *  \return true if the correct file name is set, false if base file name is empty.
 */
bool Logger::GenerateFileName()
{
    // no base set
    if (mFileNameBase.empty())
    {
        return false;
    }

    // no file yet
    if (mLogFileList.empty())
    {
        AddNewFileName();
    }
    else
    {
        // get the last file
        m_logFileName = mLogFileList[mLogFileList.size() - 1];
    }

    return true;
}

//*****************************************************************************

/*! Check the size of file and number of files
 *  and do bookkeeping
 */
void Logger::EnforceLogFileLimits()
{
    // check size
    if ((m_logFile != nullptr) && (Size() >= mMaxFileSize))
    {
        m_logFile->flush();
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;

        AddNewFileName();

        // open new one
        m_logFile = new std::ofstream(m_logFileName, (std::ios::app | std::ios::out));
    }

    // more files
    while (mLogFileList.size() > mMaxFileNumber)
    {
        unlink(mLogFileList[0].c_str());
        mLogFileList.erase(mLogFileList.begin());
    };
}

//******************************************************************************

void Logger::writeLog(std::stringstream &ss)
{
    if (mUsingStdOut)
    {
        std::cout << ss.str();
    }

    // Write to log file only if one exists and is open.
    if ((m_logFile != nullptr) && m_logFile->is_open())
    {
        *m_logFile << ss.str();
        m_logFile->flush();
    }

    // Empty the stringstream after use
    ss.str(std::string());
    ss.clear();

    // Check the file size and change the file if necessary
    EnforceLogFileLimits();
}

//*****************************************************************************

// Return date string in the form: yyyy-mm-dd or yyyymmdd
std::string Logger::GetDateStr(bool useSep)
{
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char tsBuf[16];

    if (useSep)
    {
        // Format: YYYY-MM-DD
        (void)std::strftime(tsBuf, sizeof(tsBuf), "%F", local_time);
    }
    else
    {
        // Format: YYYYMMDD
        (void)std::strftime(tsBuf, sizeof(tsBuf), "%Y%m%d", local_time);
    }

    return std::string(tsBuf);
}

//*****************************************************************************

// Return time string in the form: HH:MM:SS.nnn, HHMMSS.nnn, HH:MM:SS, or HHMMSS
std::string Logger::GetTimeStr(bool useSep, bool useMsec)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int msec = (int)((ts.tv_nsec / 1000000) % 1000);

    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char tsBuf[32];
    int idx = 0;

    if (useSep)
    {
        // Format: HH:MM:SS
        idx = std::strftime(tsBuf, sizeof(tsBuf), "%T", local_time);
    }
    else
    {
        // Format: HHMMSS
        idx = std::strftime(tsBuf, sizeof(tsBuf), "%H%M%S", local_time);
    }

    // Append milliseconds
    if (useMsec)
    {
        snprintf(&tsBuf[idx], sizeof(tsBuf), ".%03d", msec);
    }

    return std::string(tsBuf);
}

//*****************************************************************************

// Return timestamp in the form: "yyyy-mm-dd HH:MM:SS.nnn"
std::string Logger::TimeStamp(bool useSep)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int msec = (int)((ts.tv_nsec / 1000000) % 1000);

    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char tsBuf[32];
    int idx = 0;

    if (useSep)
    {
        // Format: YYYY-MM-DD HH:MM:SS
        idx = std::strftime(tsBuf, sizeof(tsBuf), "%F %T", local_time);
    }
    else
    {
        // Format: YYYYMMDD HHMMSS
        idx = std::strftime(tsBuf, sizeof(tsBuf), "%Y%m%d %H%M%S", local_time);
    }

    // Append milliseconds
    snprintf(&tsBuf[idx], sizeof(tsBuf), ".%03d", msec);

    return std::string(tsBuf);
}

//******************************************************************************

std::string Logger::severityToString(Severity s)
{
    std::stringstream name;
    if ((s >= eLOG_CRIT) && (s <= eLOG_LAST))
    {
        name << std::setfill(' ') << std::setw(4) << std::left << s_severityString[s];
    }
    return name.str();
}

//*****************************************************************************

/*! Get the oldest string in the buffer (remove from head.)
 *  This function returns NULL if the buffer is empty.
 *
 *  \return a string from buffer, null if empty.
 */
std::string Logger::Get()
{
    // nothing in the buffer
    if (mBufferList.empty())
    {
        return NULL;
    }

    // copy the line & remove from buffer
    std::string line = mBufferList.at(0);
    mBufferList.erase(mBufferList.begin());
    return line;
}

//******************************************************************************
