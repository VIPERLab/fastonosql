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

#include "core/db/lmdb/config.h"

#include <stddef.h>  // for size_t
#include <string.h>  // for strcmp

#include <string>  // for string, basic_string
#include <vector>  // for vector

#include <lmdb.h>  // for mdb_txn_abort, MDB_val

extern "C" {
#include "sds.h"
}

#include <common/convert2string.h>
#include <common/file_system.h>  // for prepare_path
#include <common/log_levels.h>   // for LEVEL_LOG::L_WARNING
#include <common/sprintf.h>      // for MemSPrintf

#include "core/logger.h"

namespace fastonosql {
namespace core {
namespace lmdb {

namespace {

Config parseOptions(int argc, char** argv) {
  Config cfg;
  for (int i = 0; i < argc; i++) {
    const bool lastarg = i == argc - 1;

    if (!strcmp(argv[i], "-d") && !lastarg) {
      cfg.delimiter = argv[++i];
    } else if (!strcmp(argv[i], "-ns") && !lastarg) {
      cfg.ns_separator = argv[++i];
    } else if (!strcmp(argv[i], "-f") && !lastarg) {
      cfg.dbname = argv[++i];
    } else if (!strcmp(argv[i], "-e") && !lastarg) {
      cfg.env_flags = common::ConvertFromString<int>(argv[++i]);
    } else {
      if (argv[i][0] == '-') {
        const std::string buff = common::MemSPrintf(
            "Unrecognized option or bad number of args "
            "for: '%s'",
            argv[i]);
        LOG_CORE_MSG(buff, common::logging::L_WARNING, true);
        break;
      } else {
        /* Likely the command name, stop here. */
        break;
      }
    }
  }
  return cfg;
}

}  // namespace

Config::Config()
    : LocalConfig(common::file_system::prepare_path("~/test.lmdb")),
      env_flags(LMDB_DEFAULT_ENV_FLAGS) {}

bool Config::ReadOnlyDB() const {
  return env_flags & MDB_RDONLY;
}

void Config::SetReadOnlyDB(bool ro) {
  if (ro) {
    env_flags |= MDB_RDONLY;
  } else {
    env_flags &= ~MDB_RDONLY;
  }
}

}  // namespace lmdb
}  // namespace core
}  // namespace fastonosql

namespace common {

std::string ConvertToString(const fastonosql::core::lmdb::Config& conf) {
  std::vector<std::string> argv = conf.Args();

  if (conf.env_flags != LMDB_DEFAULT_ENV_FLAGS) {
    argv.push_back("-e");
    argv.push_back(common::ConvertToString(conf.env_flags));
  }

  return fastonosql::core::ConvertToStringConfigArgs(argv);
}

template <>
fastonosql::core::lmdb::Config ConvertFromString(const std::string& line) {
  int argc = 0;
  sds* argv = sdssplitargslong(line.c_str(), &argc);
  if (argv) {
    auto cfg = fastonosql::core::lmdb::parseOptions(argc, argv);
    sdsfreesplitres(argv, argc);
    return cfg;
  }

  return fastonosql::core::lmdb::Config();
}

}  // namespace common
