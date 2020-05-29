/**
 * This file is part of packeteer.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#include <iostream>
#include <map>

#include <clipp.h>

#include "backends.h"


namespace {


struct backend_meta
{
  std::string               name;
  std::vector<std::string>  matches;
};

using backend_map = std::map<
    backends,
    std::pair<backend_meta, backend_ops *>
>;

static backend_map REGISTERED_BACKENDS = backend_map{};


inline backends
select_backend(std::string const & val)
{
  // Lowercase val
  std::string lower = val;
  std::transform(val.begin(), val.end(), lower.begin(),
      [](std::string::value_type c) { return std::tolower(c); });

  for (auto & it : REGISTERED_BACKENDS) {
    auto & meta = it.second.first;
    for (auto & match : meta.matches) {
      if (match == lower) {
        return it.first;
      }
    }
  }

  throw std::runtime_error("Unknown backend selected.");
}



options parse_cli(int argc, char **argv)
{
  using namespace clipp;

  bool help = false;
  options opts;

  auto cli = (
      opt_value("backend")
        .call([&](std::string const & value) {
          opts.backend = select_backend(value);
        })
        .doc("Select the backend to run the benchmark with. Possible values "
          "are: packeteer, libevent, TODO"
         ),
      option("--num-conns", "-n")
        .doc("The number of conns to use in the test.")
        & value("connectors", opts.conns),
      option("--active-conns", "-a")
        .doc("The number of active conns at any given interval.")
        & value("active", opts.active),
      option("--writes", "-w")
        .doc("The number total writes to perform.")
        & value("writes", opts.writes),

      option("-v", "--verbose")
        .set(opts.verbose)
        .doc("Be verbose."),

      option("--help", "-?")
        .set(help)
        .doc("Display this help.")
  );

  if (!parse(argc, argv, cli) || help) {
    auto fmt = doc_formatting{}
        .first_column(3)
        .last_column(79);
    std::cerr << make_man_page(cli, argv[0], fmt);
    exit(1);
  }

  if (opts.verbose) {
    std::cout << "Summary of options:" << std::endl;
    std::cout << "  Selected backend:     " << opts.backend << std::endl;
    std::cout << "  Number of connectors: " << opts.conns << std::endl;
    std::cout << "  Active connectors:    " << opts.active << std::endl;
    std::cout << "  Number of writes:     " << opts.writes << std::endl;
  }

  return opts;
}


} // anonymous namespace



std::ostream & operator<<(std::ostream & os, backends const & b)
{
  auto found = REGISTERED_BACKENDS.find(b);
  if (found == REGISTERED_BACKENDS.end()) {
    throw std::runtime_error("Backend not registered.");
  }

  os << found->second.first.name;
  return os;
}


void register_backend(backends b,
    std::string const & name,
    std::vector<std::string> const & matches,
    backend_ops * ops)
{
  backend_meta meta{name, matches};
  REGISTERED_BACKENDS[b] = std::make_pair(meta, ops);
}



int main(int argc, char **argv)
{
  // Parse CLI
  auto opts = parse_cli(argc, argv);

  auto & backend = REGISTERED_BACKENDS[opts.backend].second;

  backend->init(opts);
}
