// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 26/01/18.
// Modified by Subhagato Dutta on 07/02/18
//
//*****************************************************************

#ifndef LOGGER_H
#define LOGGER_H

#define ENABLE_LOGGING 1

#define PRINT_TO_TEXT_FILE
#define PRINT_TO_HTML_FILE

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

//#define CONVERT_ERROR_TO_ASSERT
#ifdef CONVERT_ERROR_TO_ASSERT
#define NO_ASSERT_IN_ERROR 0
#else
#define NO_ASSERT_IN_ERROR 1
#endif

#if ENABLE_LOGGING

#define CBLACK ((char)130)
#define CRED ((char)131)
#define CGREEN ((char)132)
#define CYELLOW ((char)133)
#define CBLUE ((char)134)
#define CMAGENTA ((char)135)
#define CCYAN ((char)136)
#define CWHITE ((char)137)
#define CRESET ((char)138)

#define SB_ ((char)139)
#define _SB ((char)140)
#define SI_ ((char)141)
#define _SI ((char)142)
#define SU_ ((char)143)
#define _SU ((char)144)

static std::vector<std::pair<std::string, std::string>> color_mapping = {{"\e[30m", "<font color=\"black\">"},
                                                                         {"\e[31m", "<font color=\"red\">"},
                                                                         {"\e[32m", "<font color=\"green\">"},
                                                                         {"\e[33m", "<font color=\"yellow\">"},
                                                                         {"\e[34m", "<font color=\"blue\">"},
                                                                         {"\e[35m", "<font color=\"magenta\">"},
                                                                         {"\e[36m", "<font color=\"cyan\">"},
                                                                         {"\e[37m", "<font color=\"white\">"},
                                                                         {"\e[0m", "</font>"},
                                                                         {"\e[1m", "<b>"},
                                                                         {"\e[21m", "</b>"},
                                                                         {"\e[3m", "<i>"},
                                                                         {"\e[23m", "</i>"},
                                                                         {"\e[4m", "<u>"},
                                                                         {"\e[24m", "</u>"}};

// log files
#define TEXTLOG "TraderBotLog.txt"
#define HTMLLOG "TraderBotLog.html"

class Logger {
 private:
  // private constructor and destructor
  Logger() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
      fprintf(stdout, "Current working directory: %s\n", cwd);
    else {
      // should not reach here
      perror("getcwd() error");
      exit(1);
    }

    m_cur_dir = cwd;

    const char* archive = getenv("ARCHIVE_PATH");
    m_archive = (archive ? archive : "");
    archiveLogs();

    m_text_log.open(getTextFile());
    m_html_log.open(getHtmlFile());

    printHtmlHeader();
  }

  ~Logger() {
    printHtmlFooter();
    m_text_log.close();
    m_html_log.close();

    archiveLogs();
  }

  // private copy constructor and assignment operator
  Logger(Logger&);

  Logger& operator=(Logger&);

  std::string m_cur_dir;

  std::ofstream m_text_log;
  std::ofstream m_html_log;

  std::mutex m_mutex;

  std::string m_archive;

  void printHtmlHeader() {
    m_html_log << "<html>\n";
    m_html_log << "<head>\n";
    m_html_log << "<title>TraderBot</title>\n";
    m_html_log << "</head>\n";
    m_html_log << "<body style=\"background-color:#300A24;\">"
               << "\n";
    m_html_log << color_mapping[CWHITE - CBLACK].second << "\n";
    m_html_log.flush();
  }

  void printHtmlFooter() {
    m_html_log << "</font>\n";
    m_html_log << "</body>\n";
    m_html_log << "</html>\n";
    m_html_log.flush();
  }

  void archiveLogs() {
    if (m_archive.empty()) return;

    struct stat st;
    int err = stat("controller_logs", &st);
    if (err) return;

    // get time stamp of controller_logs
    const time_t dir_creation_time = (unsigned long)st.st_mtime;
    struct tm* ptm = gmtime(&dir_creation_time);
    const std::string timestamp_str = std::to_string(ptm->tm_mday) + "-" + std::to_string(ptm->tm_mon) + "-" +
                                      std::to_string(ptm->tm_year + 1900) + "-" + std::to_string(ptm->tm_hour) + "-" +
                                      std::to_string(ptm->tm_min) + "-" + std::to_string(ptm->tm_sec);
    // sync log files with archive
    const std::string command = "rsync -av " + m_cur_dir + "/* " + m_archive + "/" + timestamp_str;
    system(command.c_str());
  }

 public:
  static Logger& getInstance() {
    static Logger sLogger;
    return sLogger;
  }

  static bool s_suppress_warnings;

  std::mutex& getMutex() {
    return m_mutex;
  }

  std::string getTextFile() {
    return (m_cur_dir + "/" + TEXTLOG);
  }

  std::string getHtmlFile() {
    return (m_cur_dir + "/" + HTMLLOG);
  }

  std::ofstream& getTextStream() {
    return m_text_log;
  }

  void printToHtml(std::string& text) {
    bool color_used = false;

    for (char& c : text) {
      if ((uint8_t)c >= 130 && (uint8_t)c <= 137) {
        if (color_used) m_html_log << "</font> ";
        m_html_log << color_mapping[(uint8_t)c - 130].second;
        color_used = true;

      } else if ((uint8_t)c >= 138 && (uint8_t)c <= 144) {
        m_html_log << color_mapping[(uint8_t)c - 130].second;
        if ((uint8_t)c == 138) color_used = false;

      } else if (c == '\r' || c == '\n') {
        m_html_log << "<br>\n";

      } else if (c == '>' || c == '<') {
        m_html_log << '|';

      } else {
        m_html_log << c;
      }
    }

    if (color_used) m_html_log << "</font>\n";

    m_html_log.flush();
  }

  void printToText(std::string& text) {
    for (auto& c : text) {
      if ((uint8_t)c < 130 || (uint8_t)c > 144) m_text_log << c;
    }

    m_text_log.flush();
  }

  bool printToTerminal(std::string& text) {
    bool color_used = false;
    for (char& c : text) {
      // printf("[%u]", (uint8_t)c);
      if ((uint8_t)c < 130 || (uint8_t)c > 144) {
        std::cout << c;
      } else {
        std::cout << color_mapping[(uint8_t)c - 130].first;
        if ((uint8_t)c != 138) color_used = true;
      }
    }

    if (color_used) std::cout << "\e[0m";  // clear formatting in the end

    std::cout << std::flush;
    return color_used;
  }
};

struct PRINT {
  std::stringstream ss;
  bool m_print;
  int m_error_code;

  PRINT(bool print = true, int error_code = 0) : m_print(print), m_error_code(error_code) {}

  ~PRINT() {
    Logger& logger_inst = Logger::getInstance();

    std::unique_lock<std::mutex> mlock(logger_inst.getMutex());

    std::string text = ss.str();

    if (m_print) {
      logger_inst.printToTerminal(text);

#ifdef PRINT_TO_HTML_FILE
      logger_inst.printToHtml(text);
#endif

#ifdef PRINT_TO_TEXT_FILE
      logger_inst.printToText(text);
#endif
    }
    if (m_error_code) {
      mlock.unlock();
      assert(NO_ASSERT_IN_ERROR);
      exit(m_error_code);
    }
  }
};

//#define _INFO (X(), std::cout << __FILE__ << " " << __LINE__ << " ")

// can print color, for important messages, doesn't print in text file
#define COUT (PRINT().ss)

// can print color, for debug info
#define CT_INFO (PRINT(!Logger::s_suppress_warnings).ss << "INFO: ")

// can't print separate color
#define CT_CRIT_WARN (PRINT().ss << CYELLOW << "WARNING: ")
#define CT_WARN (PRINT(!Logger::s_suppress_warnings).ss << CYELLOW << "WARNING: ")

// exits the program, can't print in separate color
#define CT_ERROR(error_code, exit_code) \
  (PRINT(true, static_cast<int>(exit_code)).ss << CRED << "ERROR-" << static_cast<int>(error_code) << ": ")

#else  // Logger disabled

#define CBLACK "\e[30m"
#define CRED "\e[31m"
#define CGREEN "\e[32m"
#define CYELLOW "\e[33m"
#define CBLUE "\e[34m"
#define CMAGENTA "\e[35m"
#define CCYAN "\e[36m"
#define CWHITE "\e[37m"
#define CRESET "\e[0m"

#define SB_ "\e[1m"
#define _SB "\e[21m"
#define SI_ "\e[3m"
#define _SI "\e[23m"
#define SU_ "\e[4m"
#define _SU "\e[24m"

struct EXIT {
  int m_code;
  EXIT(int code = 0) : m_code(code) {}
  ~EXIT() {
    std::cout << CRESET;
    if (m_code) exit(m_code);
  }
};

#define COUT (EXIT(), std::cout)                 // can print color, for important messages
#define CT_INFO (EXIT(), std::cout << "INFO: ")  // can print color, for debug info

// can't print separate color
#define CT_CRIT_WARN (EXIT(), std::cout << CYELLOW << "WARNING: ")

// can't suppress warning if logger is disabled
#define CT_WARN CT_CRIT_WARN

// exits the program, can't print in separate color
#define CT_ERROR(error_code, exit_code) \
  (EXIT(static_cast<int>(exit_code)), std::cout << CRED << "ERROR-" << static_cast<int>(error_code) << ": ")

#endif

#endif  // LOGGER_H
