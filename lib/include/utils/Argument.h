// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 25/9/17.
//
//*****************************************************************

#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "Globals.h"
#include "utils/ErrorHandling.h"
#include <cassert>
#include <unordered_map>
#include <unordered_set>

class ArgumentProperties {
 public:
  ArgumentProperties(const std::string& a_description, void (*a_fn)(std::string), const bool a_is_switch)
      : m_description(a_description), m_foo(a_fn), m_is_switch(a_is_switch) {}

  ~ArgumentProperties() {}

  bool isSwitch() const {
    return m_is_switch;
  }

  // call the corresponding function
  void execute(std::string& a_fn_input) const {
    m_foo(a_fn_input);
  }

  void pr() const {
    COUT << m_description << std::endl;
  }

 private:
  std::string m_description;

  // functions to call when this argument is specified for a particular type
  void (*m_foo)(std::string);

  bool m_is_switch;
};

class Argument {
 private:
  std::string m_large_arg;
  std::string m_short_arg;

  // properties : "r responses.dump"
  std::unordered_map<std::string, ArgumentProperties*> m_properties;

 public:
  Argument(const std::string& a_large_arg, const std::string& a_short_arg, const std::string& a_description,
           void (*a_fn)(std::string), const bool a_is_switch, const std::string& a_secondary_arg)
      : m_large_arg(a_large_arg), m_short_arg(a_short_arg) {
    // first 2 letters have be "--" for the long argument
    assert(m_large_arg.substr(0, 2) == "--");

    addSecondaryArg(a_description, a_fn, a_is_switch, a_secondary_arg);
  }

  ~Argument() {
    for (auto& properties : m_properties) {
      DELETE(properties.second);
    }
    m_properties.clear();
  }

  void addSecondaryArg(const std::string& a_description, void (*a_fn)(std::string), const bool a_is_switch,
                       const std::string& a_secondary_arg) {
    // duplicate secondary argument not allowed
    assert(m_properties.find(a_secondary_arg) == m_properties.end());

    if (m_properties.size() > 0) {
      // If one secondary argument is present,
      // "" can no longer be a secondary argument.
      assert(m_properties.find("") == m_properties.end());
    }

    ArgumentProperties* properties = new ArgumentProperties(a_description, a_fn, a_is_switch);
    m_properties[a_secondary_arg] = properties;
  }

  bool execute(std::string& a_fn_input, const std::string& a_secondary_arg = "") {
    if (m_properties.find(a_secondary_arg) == m_properties.end()) return false;

    const ArgumentProperties* properties = m_properties[a_secondary_arg];
    properties->execute(a_fn_input);

    return true;
  }

  bool hasSecondaryArgs() const {
    return (m_properties.find("") == m_properties.end());
  }

  bool isSwitch(const std::string& a_secondary_arg = "") {
    assert(m_properties.find(a_secondary_arg) != m_properties.end());
    return m_properties[a_secondary_arg]->isSwitch();
  }

  void pr() {
    COUT << m_large_arg + " / " + m_short_arg + " : \n";

    for (auto& properties : m_properties) {
      std::string a_secondary_arg = properties.first;
      ArgumentProperties* property = properties.second;

      COUT << "\t" << a_secondary_arg << " : ";
      property->pr();
    }
  }
};

class ArgumentParser {
 private:
  std::unordered_map<std::string, Argument*> m_arguments;

 public:
  ArgumentParser() {}

  ~ArgumentParser() {
    std::unordered_set<Argument*> deleted_args;

    for (auto& args : m_arguments) {
      Argument* arg = args.second;
      if (!arg) continue;

      if (deleted_args.find(arg) != deleted_args.end()) continue;

      deleted_args.insert(arg);
      DELETE(arg);
    }
    m_arguments.clear();
  }

  // a_large_arg: descriptive argument
  // a_short_arg: short argument
  // a_description: description
  // isSwitch: true if in case it is a word argument like --fixCoinMarketCapData,
  //					it can be true also when 2nd argument consist only one word like "--dump r"
  // foo: function to execute when this argument is present
  // a_secondary_arg: 2nd argument w.r.t the current argument like "--dump r"
  void addArguments(const std::string& a_large_arg, const std::string& a_short_arg, const std::string& a_description,
                    const bool isSwitch, void (*foo)(std::string), const std::string& a_secondary_arg = "") {
    assert(m_arguments[a_large_arg] == m_arguments[a_short_arg]);

    Argument* arg = m_arguments[a_large_arg];
    if (arg == NULL) {
      Argument* arg = new Argument(a_large_arg, a_short_arg, a_description, foo, isSwitch, a_secondary_arg);
      m_arguments[a_large_arg] = arg;
      m_arguments[a_short_arg] = arg;
    } else
      arg->addSecondaryArg(a_description, foo, isSwitch, a_secondary_arg);
  }

  void processArguments(const int argc, const char** argv) {
    for (int argIdx = 0; argIdx < argc; ++argIdx) {
      std::string arg_str = argv[argIdx];

      if ((arg_str == "--help") || (arg_str == "-h")) {
        // print help and exit

        COUT << "\n";

        for (auto& args : m_arguments) {
          std::string command = args.first;
          Argument* arg = args.second;

          if (command.substr(0, 2) == "--") arg->pr();
        }

        // Logger::getInstance()->exitLogger();
        exit(0);
      }

      if (m_arguments.find(arg_str) != m_arguments.end()) {
        Argument* arg = m_arguments[arg_str];

        std::string secondary_arg = "";

        if (arg->hasSecondaryArgs()) {
          if (argIdx >= (argc - 1)) {
            INCOMPLETE_ARGUMENT_ERROR(arg_str, "");
          }

          secondary_arg = argv[argIdx + 1];
          argIdx++;
        }

        std::string fn_input = "";

        if (!arg->isSwitch(secondary_arg)) {
          if (argIdx >= (argc - 1)) {
            INCOMPLETE_ARGUMENT_ERROR(arg_str, secondary_arg);
          }

          fn_input = argv[argIdx + 1];
          argIdx++;
        }

        bool success = arg->execute(fn_input, secondary_arg);
        if (!success) {
          INVALID_ARGUMENT_ERROR(arg_str, secondary_arg);
        }
      } else {
        // check for multi-letter combined argument

        if ((arg_str.length() > 1) && (arg_str[0] == '-')) {
          bool valid_arguments = true;

          for (int charIdx = (arg_str.length() - 1); charIdx > 0; --charIdx) {
            std::string switch_arg = "-" + std::string(1, arg_str[charIdx]);

            if (m_arguments.find(switch_arg) == m_arguments.end()) {
              valid_arguments = false;
              break;
            }

            Argument* arg = m_arguments[switch_arg];
            if (arg->hasSecondaryArgs() || !arg->isSwitch()) {
              valid_arguments = false;
              break;
            }

            std::string a_fn_input = "";
            bool success = arg->execute(a_fn_input);
            assert(success);
          }

          if (valid_arguments) continue;
        }

        INVALID_ARGUMENT_ERROR(arg_str, "");
      }
    }
  }
};

#endif  // ARGUMENT_H
