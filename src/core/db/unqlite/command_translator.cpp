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

#include "core/db/unqlite/command_translator.h"

#include <common/macros.h>  // for UNUSED
#include <common/sprintf.h>

#define UNQLITE_COMMONTYPE_GET_KEY_COMMAND COMMONTYPE_GET_KEY_COMMAND

#define UNQLITE_SET_KEY_PATTERN_2ARGS_SS "SET %s %s"
#define UNQLITE_GET_KEY_PATTERN_1ARGS_S UNQLITE_COMMONTYPE_GET_KEY_COMMAND " %s"
#define UNQLITE_DELETE_KEY_PATTERN_1ARGS_S "DEL %s"
#define UNQLITE_RENAME_KEY_PATTERN_2ARGS_SS "RENAME %s %s"

namespace fastonosql {
namespace core {
namespace unqlite {

CommandTranslator::CommandTranslator(const std::vector<CommandHolder>& commands)
    : ICommandTranslator(commands) {}

common::Error CommandTranslator::CreateKeyCommandImpl(const NDbKValue& key,
                                                      std::string* cmdstring) const {
  std::string key_str = key.KeyString();
  std::string value_str = key.ValueString();
  *cmdstring = common::MemSPrintf(UNQLITE_SET_KEY_PATTERN_2ARGS_SS, key_str, value_str);
  return common::Error();
}

common::Error CommandTranslator::LoadKeyCommandImpl(const NKey& key,
                                                    common::Value::Type type,
                                                    std::string* cmdstring) const {
  UNUSED(type);

  std::string key_str = key.Key();
  *cmdstring = common::MemSPrintf(UNQLITE_GET_KEY_PATTERN_1ARGS_S, key_str);
  return common::Error();
}

common::Error CommandTranslator::DeleteKeyCommandImpl(const NKey& key,
                                                      std::string* cmdstring) const {
  std::string key_str = key.Key();
  *cmdstring = common::MemSPrintf(UNQLITE_DELETE_KEY_PATTERN_1ARGS_S, key_str);
  return common::Error();
}

common::Error CommandTranslator::RenameKeyCommandImpl(const NKey& key,
                                                      const std::string& new_name,
                                                      std::string* cmdstring) const {
  std::string key_str = key.Key();
  *cmdstring = common::MemSPrintf(UNQLITE_RENAME_KEY_PATTERN_2ARGS_SS, key_str, new_name);
  return common::Error();
}

common::Error CommandTranslator::ChangeKeyTTLCommandImpl(const NKey& key,
                                                         ttl_t ttl,
                                                         std::string* cmdstring) const {
  UNUSED(key);
  UNUSED(ttl);
  UNUSED(cmdstring);

  std::string errorMsg = common::MemSPrintf("Sorry, but now " PROJECT_NAME_TITLE
                                            " not supported change ttl command for UnqLite.");
  return common::make_error_value(errorMsg, common::ErrorValue::E_ERROR);
}

common::Error CommandTranslator::LoadKeyTTLCommandImpl(const NKey& key,
                                                       std::string* cmdstring) const {
  UNUSED(key);
  UNUSED(cmdstring);

  std::string errorMsg = common::MemSPrintf("Sorry, but now " PROJECT_NAME_TITLE
                                            " not supported get ttl command for UnqLite.");
  return common::make_error_value(errorMsg, common::ErrorValue::E_ERROR);
}

bool CommandTranslator::IsLoadKeyCommandImpl(const CommandInfo& cmd) const {
  return cmd.IsEqualName(UNQLITE_COMMONTYPE_GET_KEY_COMMAND);
}

common::Error CommandTranslator::PublishCommandImpl(const NDbPSChannel& channel,
                                                    const std::string& message,
                                                    std::string* cmdstring) const {
  UNUSED(channel);
  UNUSED(message);
  UNUSED(cmdstring);

  std::string errorMsg = common::MemSPrintf("Sorry, but now " PROJECT_NAME_TITLE
                                            " not supported publish command for UnqLite.");
  return common::make_error_value(errorMsg, common::ErrorValue::E_ERROR);
}

common::Error CommandTranslator::SubscribeCommandImpl(const NDbPSChannel& channel,
                                                      std::string* cmdstring) const {
  UNUSED(channel);
  UNUSED(cmdstring);

  std::string errorMsg = common::MemSPrintf("Sorry, but now " PROJECT_NAME_TITLE
                                            " not supported subscribe command for UnqLite.");
  return common::make_error_value(errorMsg, common::ErrorValue::E_ERROR);
}

}  // namespace unqlite
}  // namespace core
}  // namespace fastonosql
