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
 * Logger.h
 *
 *  Created on: Aug 10, 2011
 *      Author: J. Parziale
 *
 *  History:
 *  00 08/10/2011
 *    Original version.
 */

#ifndef LOGGER_H_
#define LOGGER_H_
//******************************************************************************

#include <iostream>
#include <fstream>
#include <mutex> // std::mutex, std::unique_lock
#include <sstream>
#include <memory>
#include <vector>
#include <map>

#include "version.h"

#ifndef nullptr
#define nullptr NULL
#endif

//******************************************************************************

/*! Predefined verbosity levels
 *  Mirroring values from syslog.h
 */
enum Severity
{
    eLOG_CRIT = 2, //!< Critical messages only
    eLOG_HIGH,     //!< High importance
    eLOG_MED,      //!< Medium importance
    eLOG_LOW,      //!< Low importance
    eLOG_INFO,     //!< Informational
    eLOG_DEBUG,    //!< Debug level - always go to console.
    eLOG_LAST = eLOG_DEBUG,
    eLOG_DEFAULT = eLOG_MED
};
#if 0
// From syslog.h:
#define LOG_EMERG 0   /* system is unusable */
#define LOG_ALERT 1   /* action must be taken immediately */
#define LOG_CRIT 2    /* critical conditions */
#define LOG_ERR 3     /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE 5  /* normal but significant condition */
#define LOG_INFO 6    /* informational */
#define LOG_DEBUG 7   /* debug-level messages */
#endif

//! Define the size of the buffer (number of lines to buffer before flushing to output)
const unsigned int LOG_BUF_LEN = 250;

//! Set default max log file size
const unsigned long DEFAULT_MAX_FILE_SIZE = (2 * 1024 * 1024); // 2MB

//! Set default the max number of log files
const unsigned long DEFAULT_MAX_FILE_NUMBER = 128;

//******************************************************************************

/**
 * Class to provide logging capabilities.
 */
class Logger : public std::enable_shared_from_this<Logger>
{
public:
    /**
     * Constructor to initialize the logger object.
     * Log output will be sent to standard output.
     * @param severity  Severity level to restrict output.
     */
    Logger(Severity severity = eLOG_DEFAULT);

    /**
     * Constructor to initialize the logger object.
     * Log output will be sent to a set of files with the specified name as base,
     * appended with the date and time of when a particular file is opened.
     * @param severity Severity level to restrict output.
     * @param baseName Name of the log file to open.
     */
    explicit Logger(const std::string baseName, Severity severity = eLOG_DEFAULT);

    /**
     * Destructor needs to specifically close the log output.
     */
    ~Logger();

    /* ------------------------------------------------------------------------ */

    /**
     * Write the formatted string to log file.
     * @param s Severity level of this message.
     * @param logline Format string.
     */
    static void log(Severity s, const char *logline, ...);

    /**
     * Write the input string to log file (with newline).
     * @param s Severity level of this message.
     * @param input  The string to print
     */
    static void log(Severity s, const std::string &input);

    /**
     * Write the input string to log file (with newline).
     * @param s Severity level of this message.
     * @param input  The stringstream to print
     * Side effect: the input stringstream will be cleared.
     */
    static void log(Severity s, std::stringstream &input);

    /**
     * Get pointer to instance of this singleton.
     */
    static Logger *getInstance();

    /**
     * Enable sending all logs to console.
     */
    static void MirrorToStdOut()
    {
        mUsingStdOut = true;
    }

    /**
     *  Return max log file size
     */
    unsigned long MaxFileSize()
    {
        return mMaxFileSize;
    };

    /**
     * Return max number of log files
     */
    unsigned long MaxFileNumber()
    {
        return mMaxFileNumber;
    };

    /**
     * Start writing logs to buffer.
     */
    void SetBuffering(bool on);

    /**
     * Flush the buffer to the disk (only useful if buffering)
     */
    void WriteToFile();

    /**
     * Set log base name
     * It is formatted like so: <basename>_YYYYMMDD_HHMMSS.log
     */
    bool SetBaseName(const std::string baseName)
    {
        mFileNameBase = baseName;
        return MakeFileList();
    };

    /**
     * Set max log file size
     */
    void SetMaxFileSize(unsigned long size)
    {
        mMaxFileSize = size;
    };

    /**
     * Set the max number of log files
     */
    void SetMaxFileNumber(unsigned long size)
    {
        mMaxFileNumber = size;
    };

    /**
     * Set and get the logging severity level.
     */
    static bool setSeverity(Severity s);
    static Severity getSeverity()
    {
        return m_leastSeverity;
    };

    /**
     * Return False if the entry will not be logged due to its level
     */
    static inline bool isDisplayed(const Severity s)
    {
        return (s <= m_leastSeverity);
    }

    /**
     * Flush output to file.
     */
    static void Flush()
    {
        std::cout.flush();
        if (m_logFile != NULL)
        {
            m_logFile->flush();
        }
    }

    /**
     * Return file size
     */
    static std::size_t Size()
    {
        std::ifstream in(m_logFileName.c_str(), std::ios::in | std::ios::ate | std::ios::binary);
        in.seekg(0, std::ios::end);
        return in.tellg();
    }

    /**
     * Return name of log file or output stream.
     * @return String containing log file description.
     */
    static inline std::string name()
    {
        return m_logFileName;
    };

    /* ------------------------------------------------------------------------ */

    static std::string GetDateStr(bool useSep = true);
    static std::string GetTimeStr(bool useSep = true, bool useMsec = true);
    static std::string TimeStamp(bool useSep = true);

    static std::string severityToString(Severity s);

    /* ------------------------------------------------------------------------ */

private:
    // Open the last log file
    static bool Open(std::ios::openmode mode = std::ios::app | std::ios::out);

    // Create log file list
    static bool MakeFileList();

    // Get the most recent log file name or create one if necessary
    static bool GenerateFileName();

    static void writeLog(std::stringstream &ss);

    // Check the size of the log file and the number of log files
    static void EnforceLogFileLimits();

    // Make New FileName and add to the list
    static void AddNewFileName();

    // Get the oldest string in the buffer.
    static std::string Get();

    static Logger *s_pInstance;

    static std::ofstream *m_logFile;
    static Severity m_leastSeverity;
    static std::mutex m_mutex;

    static std::stringstream m_outString;

    static std::map<Severity, std::string> s_severityString;

    // Name of current log file
    static std::string m_logFileName;

    // Base name for log file, it should have full path
    static std::string mFileNameBase;

    // The max log file size
    static unsigned int mMaxFileSize;

    // The max number of log files
    static unsigned int mMaxFileNumber;

    // Header string
    static std::string mHeader;

    // Buffering flag
    static bool mBuffering;

    // The list of log files
    static std::vector<std::string> mLogFileList;

    // Log Buffer
    static std::vector<std::string> mBufferList;

    // Set to True to mirror output to console.
    static bool mUsingStdOut;

    /* ------------------------------------------------------------------------ */
};

//******************************************************************************
#endif /* LOGGER_H_ */
