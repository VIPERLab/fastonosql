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

#include "core/db/memcached/config.h"

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint16_t
#include <string.h>  // for strcmp

#include <string>  // for string, basic_string
#include <vector>  // for vector

extern "C" {
#include "sds.h"
}

#include <common/convert2string.h>  // for ConvertFromString
#include <common/log_levels.h>      // for LEVEL_LOG::L_WARNING
#include <common/net/types.h>       // for HostAndPort
#include <common/sprintf.h>         // for MemSPrintf

#include "core/logger.h"

#define DEFAULT_MEMCACHED_SERVER_PORT 11211

namespace fastonosql {
namespace core {
namespace memcached {

namespace {

Config parseOptions(int argc, char** argv) {
  Config cfg;
  for (int i = 0; i < argc; i++) {
    const bool lastarg = i == argc - 1;

    if (!strcmp(argv[i], "-h") && !lastarg) {
      cfg.host.host = argv[++i];
    } else if (!strcmp(argv[i], "-p") && !lastarg) {
      cfg.host.port = common::ConvertFromString<uint16_t>(argv[++i]);
    } else if (!strcmp(argv[i], "-u") && !lastarg) {
      cfg.user = argv[++i];
    } else if (!strcmp(argv[i], "-a") && !lastarg) {
      cfg.password = argv[++i];
    } else if (!strcmp(argv[i], "-d") && !lastarg) {
      cfg.delimiter = argv[++i];
    } else if (!strcmp(argv[i], "-ns") && !lastarg) {
      cfg.ns_separator = argv[++i];
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
    : RemoteConfig(common::net::HostAndPort::createLocalHost(DEFAULT_MEMCACHED_SERVER_PORT)),
      user(),
      password() {}

}  // namespace memcached
}  // namespace core
}  // namespace fastonosql

namespace common {

std::string ConvertToString(const fastonosql::core::memcached::Config& conf) {
  std::vector<std::string> argv = conf.Args();

  if (!conf.user.empty()) {
    argv.push_back("-u");
    argv.push_back(conf.user);
  }

  if (!conf.password.empty()) {
    argv.push_back("-a");
    argv.push_back(conf.password);
  }

  return fastonosql::core::ConvertToStringConfigArgs(argv);
}

template <>
fastonosql::core::memcached::Config ConvertFromString(const std::string& line) {
  int argc = 0;
  sds* argv = sdssplitargslong(line.c_str(), &argc);
  if (argv) {
    auto cfg = fastonosql::core::memcached::parseOptions(argc, argv);
    sdsfreesplitres(argv, argc);
    return cfg;
  }

  return fastonosql::core::memcached::Config();
}

}  // namespace common
