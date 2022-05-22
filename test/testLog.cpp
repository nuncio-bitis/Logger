/*
 * testLog.cpp
 *   Test/demo application for Logger class.
 *
 *  Created on: Aug 10, 2011
 *      Author: J. Parziale
 */

#include <iostream>
#include <sstream>
#include <libgen.h>
#include "Logger.h"

//******************************************************************************

void LoggerTest(Logger &logger)
{
  // Set initial severity to one level above the highest debug level.
  logger.setSeverity(eLOG_INFO);

  std::cout << "Writing message to log file, severity = INFO" << std::endl;

  logger.log(eLOG_INFO, "This is some log output for the log file...");
  logger.log(eLOG_INFO, "This is some formatted log output for the log file; %s", logger.name().c_str());

  logger.log(eLOG_INFO, "Only even numbers will print to log file:");
  std::cout << "Only even numbers will print to log file:" << std::endl;
  std::cout << "All numbers will print to screen:" << std::endl;

  std::stringstream ss;

  for (int i = 1; i <= 10; i++)
  {
    ss << "Log output " << i;

    std::cout << ss.str() << std::endl;
    logger.log(eLOG_INFO, ss.str());

    logger.log(eLOG_INFO, "Log formatted output : %03d", i);
    logger.Flush();

    ss.str("");
  }
  logger.log(eLOG_INFO, "----------------------------------------");

  std::cout << "Printing messages with every severity level..." << std::endl;
  logger.log(eLOG_INFO, "Printing messages with every severity level:");
  logger.setSeverity(eLOG_MED);
  for (int i = eLOG_CRIT; i <= eLOG_LAST; i++)
  {
    logger.log(Severity(i), "Log message; %d", i);
  }
  logger.setSeverity(eLOG_INFO);
  logger.log(eLOG_INFO, "----------------------------------------");

  std::cout << "Printing message at severity level INFO, but changing allowed severity level..." << std::endl;
  logger.log(eLOG_INFO, "Printing message at severity level INFO, but changing allowed severity level:");
  for (int i = eLOG_CRIT; i <= eLOG_LAST; i++)
  {
    logger.setSeverity(Severity(i));
    logger.log(eLOG_INFO, "Log message; %d", i);
  }
  logger.setSeverity(eLOG_INFO);
  logger.log(eLOG_INFO, "----------------------------------------");

  for (int i = -1; i < 10; i++)
  {
    if (logger.setSeverity(Severity(i)))
    {
      logger.log(Severity(i), "Log message; %d", i);
    }
    else
    {
      std::cerr << "ERROR: Bad severity level: " << i << std::endl;
      logger.log(eLOG_HIGH, "ERROR: Bad severity level: %d", i);
    }
  }

  //----------------------------------------------------------------------------
}

//******************************************************************************

int main(int argc, char *argv[])
{
  std::cout << std::endl << ">>> "
            << basename(argv[0]) << ": Using Logger v" << LOGGER_VERSION_MAJOR << "." << LOGGER_VERSION_MINOR
            << std::endl;
#if 1
  std::cout << std::endl
            << "--------------------------------------------------------------------------------" << std::endl
            << "Output to console only:"
            << std::endl
            << std::endl;

  // Test Logger class using console output.
  {
    // Logger that outputs data to the console.
    Logger screenLog(eLOG_DEBUG);
    LoggerTest(screenLog);
    std::cout << std::endl;
    std::cout << "Output should only have been to the console." << std::endl
              << std::endl;
  }
#endif

  //----------------------------------------------------------------------------

#if 1
  std::cout << std::endl
            << "--------------------------------------------------------------------------------" << std::endl
            << "Output to log file only:"
            << std::endl
            << std::endl;

  // Test Logger class using an output log file
  {
    // Path for output log file.
    std::string outputDocumentPath = "../data";

    // Base name of the output log file.
    // This file will be created in the output document directory.
    std::string debugLogFileName = "TestLog";

    // Open log file in the selected directory and turn on debugging output.
    Logger log(outputDocumentPath + "/" + debugLogFileName);

    LoggerTest(log);
    std::cout << std::endl;
    std::cout << "See output in " << log.name() << std::endl
              << std::endl;
  }
#endif

  //----------------------------------------------------------------------------

#if 0
  std::cout << std::endl
      << "--------------------------------------------------------------------------------" << std::endl
      << "Output to log file AND console:"
      << std::endl << std::endl;

  // Test Logger class using an output log file
  {
    // Path for output log file.
    std::string outputDocumentPath = ".";

    // Base name of the output log file.
    // This file will be created in the output document directory.
    std::string debugLogFileName = "TestLogConsole";

    // Open log file in the selected directory and turn on debugging output.
    Logger log(outputDocumentPath + "/" + debugLogFileName);

    log.MirrorToStdOut();

    LoggerTest (log);
    std::cout << std::endl;
    std::cout << "See output in " << log.name() << std::endl << std::endl;
  }
#endif

  //----------------------------------------------------------------------------
  return EXIT_SUCCESS;
}
