// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 9/9/17.
//

#include "TraderBot.h"
#include <wait.h>

// initialize global variables
#define DEFINE_GLOBALS
#include "Globals.h"

#if ENABLE_LOGGING
bool Logger::s_suppress_warnings = false;
#endif

#undef DEFINE_GLOBALS

using namespace std;

// globals in main.cpp
TraderBot* TraderBot::mp_handler = nullptr;

// main()
int main(const int argc, const char** argv) {
#ifdef DEBUG
  COUT << "\n...... Debug build ......\n\n";
#endif

  COUT << CGREEN << endl << "Welcome to cryptotrader" << endl;
  COUT << "=========================" << endl;

  bool retry = false;

  for (int i = 1; i < argc; i++) {
    // looking for retry argument
    if ((strcmp(argv[i], "--retry") == 0) || (strcmp(argv[i], "-rt") == 0)) {
      retry = true;
      break;
    }
  }

  if (!retry) return TraderBot::getInstance()->traderMain(argc, argv);

  int pid = -1;
  int status = 0;
  int first_run = true;

  while (true) {
    if (!first_run) {
      wait(&status);  // wait for child to exit
      COUT << CYELLOW << "========================================\n";
      CT_INFO << CRED << "Previous program status = " << status << ", " << WIFEXITED(status) << ", "
              << WEXITSTATUS(status) << endl;
      COUT << CYELLOW << "========================================\n";

      // wait for 1 sec before restarting
      this_thread::sleep_for(chrono::seconds(1));
    }

    if (first_run || !WIFEXITED(status) || (WIFEXITED(status) && WEXITSTATUS(status))) {
      first_run = false;
      pid = fork();
      if (pid == 0) {  // child process
        return TraderBot::getInstance()->traderMain(argc, argv);
      }

      if (pid < 0) {
        puts("uh... crashed and cannot restart");
        exit(1);
      }
    } else
      break;
  }

  return 0;
}
