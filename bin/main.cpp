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
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <process.h>  // _spawnv
#else
#include <wait.h>
#endif

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
#ifdef _WIN32
  // Enable ANSI escape-sequence processing so the colour codes the logger emits
  // render as colours instead of raw "\033[..m" text on the Windows console.
  {
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD console_mode = 0;
    if (h_out != INVALID_HANDLE_VALUE && GetConsoleMode(h_out, &console_mode))
      SetConsoleMode(h_out, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
#endif

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

#ifdef _WIN32
  // Windows has no fork(). Act as a supervisor instead: repeatedly launch a
  // child copy of ourselves (with the retry flag stripped so the child runs
  // traderMain directly) until it exits cleanly with status 0.
  vector<const char*> child_args;
  child_args.push_back(argv[0]);
  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "--retry") == 0) || (strcmp(argv[i], "-rt") == 0)) continue;
    child_args.push_back(argv[i]);
  }
  child_args.push_back(nullptr);

  bool first_run = true;
  while (true) {
    if (!first_run) {
      // wait for 1 sec before restarting
      this_thread::sleep_for(chrono::seconds(1));
    }
    first_run = false;

    const intptr_t status = _spawnv(_P_WAIT, argv[0], child_args.data());
    if (status == -1) {
      puts("uh... crashed and cannot restart");
      exit(1);
    }

    COUT << CYELLOW << "========================================\n";
    CT_INFO << CRED << "Previous program status = " << (int)status << endl;
    COUT << CYELLOW << "========================================\n";

    if (status == 0) break;  // clean exit, stop supervising
  }

  return 0;
#else
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
#endif
}
