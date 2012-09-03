// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/program_options.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

namespace base {

struct same_name {
  const string& name;
  same_name(const string& name) : name(name) { }
  bool operator()(const ProgramOptions::Option* a) {
    return a->name() == name;
  }
};

struct same_mnemonic {
  char mnemonic;
  same_mnemonic(char mnemonic) : mnemonic(mnemonic) { }
  bool operator()(const ProgramOptions::Option* a) {
    return a->mnemonic() == mnemonic;
  }
};

ProgramOptions::ProgramOptions()
{
}

ProgramOptions::~ProgramOptions()
{
  for (OptionList::const_iterator
         it=m_options.begin(), end=m_options.end(); it != end; ++it)
    delete *it;
}

ProgramOptions::Option& ProgramOptions::add(const string& name)
{
  Option* option = new Option(name);
  m_options.push_back(option);
  return *option;
}

void ProgramOptions::parse(int argc, const char* argv[])
{
  for (int i=1; i<argc; ++i) {
    string arg(argv[i]);

    // n = number of dashes ('-') at the beginning of the argument.
    size_t n = 0;
    for (; arg[n] == '-'; ++n)
      ;
    size_t len = arg.size()-n;

    if ((n > 0) && (len > 0)) {
      // Use mnemonics
      if (n == 1) {
        char usedBy = 0;

        for (size_t j=1; j<arg.size(); ++j) {
          OptionList::iterator it =
            find_if(m_options.begin(), m_options.end(), same_mnemonic(arg[j]));

          if (it == m_options.end()) {
            stringstream msg;
            msg << "Invalid option '-" << arg[j] << "'";
            throw InvalidProgramOption(msg.str());
          }

          Option* option = *it;
          option->setEnabled(true);

          if (option->doesRequireValue()) {
            if (usedBy != 0) {
              stringstream msg;
              msg << "You cannot use '-" << option->mnemonic()
                  << "' and '-" << usedBy << "' "
                  << "together, both options need one extra argument";
              throw InvalidProgramOptionsCombination(msg.str());
            }

            if (i+1 >= argc) {
              stringstream msg;
              msg << "Option '-" << option->mnemonic()
                  << "' needs one extra argument";
              throw ProgramOptionNeedsValue(msg.str());
            }

            // Set the value specified for this argument
            option->setValue(argv[++i]);
            usedBy = option->mnemonic();
          }
        }
      }
      // Use name
      else {
        string optionName;
        string optionValue;
        size_t equalSignPos = arg.find('=', n);

        if (equalSignPos != string::npos) {
          optionName = arg.substr(n, equalSignPos-n);
          optionValue = arg.substr(equalSignPos+1);
        }
        else {
          optionName = arg.substr(n);
        }

        OptionList::iterator it =
          find_if(m_options.begin(), m_options.end(), same_name(optionName));

        if (it == m_options.end()) {
          stringstream msg;
          msg << "Invalid option '--" << optionName << "'";
          throw InvalidProgramOption(msg.str());
        }

        Option* option = *it;
        option->setEnabled(true);

        if (option->doesRequireValue()) {
          // If the option was specified without '=', we can get the
          // value from the next argument.
          if (equalSignPos == string::npos) {
            if (i+1 >= argc) {
              stringstream msg;
              msg << "Missing value in '--" << optionName
                  << "'=VALUE option specification";
              throw ProgramOptionNeedsValue(msg.str());
            }
            optionValue = argv[++i];
          }

          // Set the value specified for this argument
          option->setValue(optionValue);
        }
      }
    }
    // Add values
    else {
      m_values.push_back(arg);
    }
  }
}

void ProgramOptions::reset()
{
  m_values.clear();
  for_each(m_options.begin(), m_options.end(), &ProgramOptions::resetOption);
}

// static
void ProgramOptions::resetOption(Option* option)
{
  option->setEnabled(false);
  option->setValue("");
}

} // namespace base

std::ostream& operator<<(std::ostream& os, const base::ProgramOptions& po)
{
  size_t maxOptionWidth = 0;
  for (base::ProgramOptions::OptionList::const_iterator
         it=po.options().begin(), end=po.options().end(); it != end; ++it) {
    const base::ProgramOptions::Option* option = *it;
    size_t optionWidth = std::min<int>(26, 6+option->name().size()+1);
    if (maxOptionWidth < optionWidth)
      maxOptionWidth = optionWidth;
  }

  for (base::ProgramOptions::OptionList::const_iterator
         it=po.options().begin(), end=po.options().end(); it != end; ++it) {
    const base::ProgramOptions::Option* option = *it;
    size_t optionWidth = 6+option->name().size()+1;

    if (option->mnemonic() != 0)
      os << setw(3) << '-' << option->mnemonic() << ", ";
    else
      os << setw(6) << ' ';
    os << "--" << option->name();

    if (!option->description().empty()) {
      bool multilines = (option->description().find('\n') != string::npos);

      if (!multilines) {
        os << setw(maxOptionWidth - optionWidth + 1) << ' ' << option->description();
      }
      else {
        istringstream s(option->description());
        std::string line;
        if (std::getline(s, line)) {
          os << setw(maxOptionWidth - optionWidth + 1) << ' ' << line << '\n';
          while (std::getline(s, line)) {
            os << setw(maxOptionWidth+2) << ' ' << line << '\n';
          }
        }
      }
    }
    os << "\n";
  }

  return os;
}
