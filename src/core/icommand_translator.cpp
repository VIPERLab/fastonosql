/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "core/icommand_translator.h"

extern "C" {
#include "sds.h"
}

#include <common/sprintf.h>
#include <common/string_util.h>

#include "core/types.h"

namespace fastonosql {
namespace core {

common::Error ParseCommands(const std::string& cmd, std::vector<std::string>* cmds) {
  if (cmd.empty()) {
    return common::make_error_value("Empty command line.", common::ErrorValue::E_ERROR);
  }

  std::vector<std::string> commands;
  size_t commands_count = common::Tokenize(cmd, "\n", &commands);
  if (!commands_count) {
    return common::make_error_value("Invaid command line.", common::ErrorValue::E_ERROR);
  }

  std::vector<std::string> stable_commands;
  for (std::string input : commands) {
    std::string stable_input = StableCommand(input);
    if (stable_input.empty()) {
      continue;
    }
    stable_commands.push_back(stable_input);
  }

  *cmds = stable_commands;
  return common::Error();
}

ICommandTranslator::ICommandTranslator(const std::vector<CommandHolder>& commands)
    : commands_(commands) {}

ICommandTranslator::~ICommandTranslator() {}

common::Error ICommandTranslator::SelectDBCommand(const std::string& name,
                                                  std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  *cmdstring = common::MemSPrintf(SELECTDB_COMMAND_1S, name);
  return common::Error();
}

common::Error ICommandTranslator::FlushDBCommand(std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  *cmdstring = FLUSHDB_COMMAND;
  return common::Error();
}

common::Error ICommandTranslator::DeleteKeyCommand(const NKey& key, std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return DeleteKeyCommandImpl(key, cmdstring);
}

common::Error ICommandTranslator::RenameKeyCommand(const NKey& key,
                                                   const std::string& new_name,
                                                   std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return RenameKeyCommandImpl(key, new_name, cmdstring);
}

common::Error ICommandTranslator::CreateKeyCommand(const NDbKValue& key,
                                                   std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return CreateKeyCommandImpl(key, cmdstring);
}

common::Error ICommandTranslator::LoadKeyCommand(const NKey& key,
                                                 common::Value::Type type,
                                                 std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return LoadKeyCommandImpl(key, type, cmdstring);
}

common::Error ICommandTranslator::ChangeKeyTTLCommand(const NKey& key,
                                                      ttl_t ttl,
                                                      std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return ChangeKeyTTLCommandImpl(key, ttl, cmdstring);
}

common::Error ICommandTranslator::LoadKeyTTLCommand(const NKey& key, std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return LoadKeyTTLCommandImpl(key, cmdstring);
}

bool ICommandTranslator::IsLoadKeyCommand(const std::string& cmd, std::string* key) const {
  if (cmd.empty() || !key) {
    return false;
  }

  int argc;
  sds* argv = sdssplitargslong(cmd.c_str(), &argc);
  if (!argv) {
    return false;
  }

  const char** standart_argv = const_cast<const char**>(argv);
  const CommandHolder* cmdh = nullptr;
  size_t off = 0;
  common::Error err = TestCommandLineArgs(argc, standart_argv, &cmdh, &off);
  if (err && err->isError()) {
    sdsfreesplitres(argv, argc);
    return false;
  }

  if (IsLoadKeyCommandImpl(*cmdh)) {
    *key = argv[off];
    sdsfreesplitres(argv, argc);
    return true;
  }

  sdsfreesplitres(argv, argc);
  return false;
}

common::Error ICommandTranslator::PublishCommand(const NDbPSChannel& channel,
                                                 const std::string& message,
                                                 std::string* cmdstring) const {
  if (!cmdstring || message.empty()) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return PublishCommandImpl(channel, message, cmdstring);
}

common::Error ICommandTranslator::SubscribeCommand(const NDbPSChannel& channel,
                                                   std::string* cmdstring) const {
  if (!cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return SubscribeCommandImpl(channel, cmdstring);
}

common::Error ICommandTranslator::InvalidInputArguments(const std::string& cmd) {
  std::string buff = common::MemSPrintf("Invalid input argument(s) for command: %s.", cmd);
  return common::make_error_value(buff, common::ErrorValue::E_ERROR);
}

common::Error ICommandTranslator::NotSupported(const std::string& cmd) {
  std::string buff = common::MemSPrintf("Not supported command: %s.", cmd);
  return common::make_error_value(buff, common::ErrorValue::E_ERROR);
}

common::Error ICommandTranslator::UnknownSequence(int argc, const char** argv) {
  std::string result;
  for (int i = 0; i < argc; ++i) {
    result += argv[i];
    if (i != argc - 1) {
      result += " ";
    }
  }
  std::string buff = common::MemSPrintf("Unknown sequence: '%s'.", result);
  return common::make_error_value(buff, common::ErrorValue::E_ERROR);
}

common::Error ICommandTranslator::FindCommand(int argc,
                                              const char** argv,
                                              const CommandHolder** info,
                                              size_t* off) const {
  if (!info || !off) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  for (size_t i = 0; i < commands_.size(); ++i) {
    const CommandHolder* cmd = &commands_[i];
    size_t loff = 0;
    if (cmd->IsCommand(argc, argv, &loff)) {
      *info = cmd;
      *off = loff;
      return common::Error();
    }
  }

  return UnknownSequence(argc, argv);
}

common::Error ICommandTranslator::TestCommandArgs(const CommandHolder* cmd,
                                                  int argc_to_call,
                                                  const char** argv_to_call) const {
  if (!cmd) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return cmd->TestArgs(argc_to_call, argv_to_call);
}

common::Error ICommandTranslator::TestCommandLine(const std::string& cmd) const {
  if (cmd.empty()) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  int argc;
  sds* argv = sdssplitargslong(cmd.c_str(), &argc);
  if (!argv) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  const char** standart_argv = const_cast<const char**>(argv);
  const CommandHolder* cmdh = nullptr;
  size_t loff = 0;
  common::Error err = TestCommandLineArgs(argc, standart_argv, &cmdh, &loff);
  if (err && err->isError()) {
    sdsfreesplitres(argv, argc);
    return err;
  }

  return common::Error();
}

common::Error ICommandTranslator::TestCommandLineArgs(int argc,
                                                      const char** argv,
                                                      const CommandHolder** info,
                                                      size_t* off) const {
  const CommandHolder* cmd = nullptr;
  size_t loff = 0;
  common::Error err = FindCommand(argc, argv, &cmd, &loff);
  if (err && err->isError()) {
    return err;
  }

  int argc_to_call = argc - loff;
  const char** argv_to_call = argv + loff;
  err = TestCommandArgs(cmd, argc_to_call, argv_to_call);
  if (err && err->isError()) {
    return err;
  }

  *info = cmd;
  *off = loff;
  return common::Error();
}

}  // namespace core
}  // namespace fastonosql
