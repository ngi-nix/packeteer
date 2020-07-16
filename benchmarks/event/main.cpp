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
#include <memory>
#include <chrono>
#include <fstream>

#include <clipp.h>

#include <packeteer/error.h>

#include "backends.h"


namespace {


struct backend_meta
{
  std::string               name;
  std::vector<std::string>  matches;
};

using backend_map = std::map<
    backends,
    std::pair<backend_meta, std::shared_ptr<backend_ops>>
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
          try {
            opts.backend = select_backend(value);
          }
          catch (std::runtime_error const& ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
            help = true;
          }
        })
        .doc("Select the backend to run the benchmark with. Possible values "
          "are: packeteer, libevent, TODO"
         ),

      option("-n", "--num-conns")
        .doc("The number of conns to use in the test.")
        & value("connectors", opts.conns),
      option("-a", "--active-conns")
        .doc("The number of active conns at any given interval.")
        & value("active", opts.active),
      option("-w", "--writes")
        .doc("The number total writes to perform.")
        & value("writes", opts.writes),

      option("-p", "--port-range-start")
        .doc("Start of the port range to use.")
        & value("start", opts.port_range_start),

      option("-r", "--runs")
        .doc("Number of test runs to perform.")
        & value("runs", opts.runs),

      option("-o", "--output")
        .doc("Output file (CSV) for test results.")
        & value("output_file", opts.output_file),

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
    std::cout << "  Start of port range:  " << opts.port_range_start << std::endl;
    std::cout << "  Test runs:            " << opts.runs << std::endl;
    std::cout << "  Output file:          " << opts.output_file << std::endl;
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
  REGISTERED_BACKENDS[b] = std::make_pair(meta,
      std::shared_ptr<backend_ops>(ops));
}



struct test_context
{
  options       opts;
  backend_ops & backend;

  size_t fired = 0;
  size_t space = 0;
  size_t send_errors = 0;
  size_t recv_errors = 0;
  size_t bytes_received = 0;

  std::map<conn_index, size_t> sent;
  std::map<conn_index, size_t> received;

  char send_buf[1] = { 'e' };

  test_context(options const & _opts, backend_ops & _backend)
    : opts(_opts)
    , backend(_backend)
  {
    space = opts.conns / opts.active;
    VERBOSE_LOG(opts, "Space between active connections: " << space);
  }


  conn_index send_index(conn_index from_idx)
  {
    return (from_idx + 1) % opts.conns;
  }


  bool send_and_count_errors(conn_index from_idx)
  {
    conn_index recipient = send_index(from_idx);
    auto success = backend.sendto(from_idx, recipient, send_buf,
        sizeof(send_buf));

    if (!success) {
      ++send_errors;
    }
    sent[recipient] += 1;
    return success;
  }


  void fire_initial_events()
  {
    for (conn_index i = 0 ; i < opts.active ; ++i, ++fired) {
      send_and_count_errors(i * space);
    }
  }


  void read_callback_impl(backend_ops & the_backend, conn_index index)
  {
    // Read from the indicated index.
    auto read = the_backend.recv(index);
    if (read >= 0) {
      bytes_received += read;
      received[index] += 1;
      VERBOSE_LOG(opts, "Received " << read << " Bytes on " << index << ".");

      // If we still have writes to perform, select a write index and write.
      if (fired < opts.writes) {
        send_and_count_errors(index);
        ++fired;
      }
    }
    else {
      if (read == -1) {
        ++recv_errors;
        VERBOSE_LOG(opts, "Error on " << index);
      }
      else if (read == -2) {
        // ERR_ASYNC
      }
      else {
        ++recv_errors;
        VERBOSE_LOG(opts, "Internal error!");
      }
    }
  }
};


void output_console(int run, int usec, test_context const & ctx)
{
  std::cout << "Run " << run << " completed in " << usec << " usec." << std::endl;
  std::cout << "  Fired:           " << ctx.fired << std::endl;
  std::cout << "  Bytes:           " << ctx.bytes_received << std::endl;
  std::cout << "  Send errors:     " << ctx.send_errors << std::endl;
  std::cout << "  Recv errors:     " << ctx.recv_errors << std::endl;

  size_t total_sent = 0;
  for (auto & [idx, n] : ctx.sent) {
    total_sent += n;
  }
  std::cout << "  Sent unique:     " << ctx.sent.size() << std::endl;
  std::cout << "  Sent total:      " << total_sent << std::endl;

  size_t total_received = 0;
  for (auto & [idx, n] : ctx.received) {
    total_received += n;
  }
  std::cout << "  Received unique: " << ctx.received.size() << std::endl;
  std::cout << "  Received total:  " << total_received << std::endl;
}


void output_csv(int run, int usec, test_context const & ctx, std::ofstream & file)
{
  file << REGISTERED_BACKENDS[ctx.opts.backend].first.name << ",";
  file << ctx.opts.conns << ",";
  file << ctx.opts.active << ",";
  file << ctx.opts.writes << ",";
  file << ctx.opts.runs << ",";

  file << run << ",";
  file << usec << ",";

  file << ctx.fired << ",";
  file << ctx.bytes_received << ",";
  file << ctx.send_errors << ",";
  file << ctx.recv_errors << ",";

  size_t total_sent = 0;
  for (auto & [idx, n] : ctx.sent) {
    total_sent += n;
  }
  file << ctx.sent.size() << ",";
  file << total_sent << ",";

  size_t total_received = 0;
  for (auto & [idx, n] : ctx.received) {
    total_received += n;
  }
  file << ctx.received.size() << ",";
  file << total_received << ",";

  file << "\n";
}


void output_csv_header(options const & opts, std::ofstream & file)
{
  file << "Backend,";
  file << "Connections,";
  file << "Active,";
  file << "Writes,";
  file << "Total Runs,";

  file << "Run,";
  file << "Time (usec),";

  file << "Fired,";
  file << "Bytes Received,";
  file << "Send Errors,";
  file << "Receive Errors,";

  file << "Sent Unique,";
  file << "Sent Total,";

  file << "Received Unique,";
  file << "Received Total,";

  file << "\n";
}



int main(int argc, char **argv)
{
  try {
    // Parse CLI
    auto opts = parse_cli(argc, argv);

    auto & backend = REGISTERED_BACKENDS[opts.backend].second;

    backend->init(opts);
    VERBOSE_LOG(opts, "Backend initialized.");

    bool io_errors = false;
    bool duplication_errors = false;

    std::ofstream output_file;
    if (!opts.output_file.empty()) {
      output_file.open(opts.output_file);
      output_csv_header(opts, output_file);
    }

    for (size_t run = 0 ; run < opts.runs ; ++run) {
      VERBOSE_LOG(opts, "=== Start of test run: " << run);

      test_context ctx{opts, *backend.get()};
      using namespace std::placeholders;
      auto func = std::bind(&test_context::read_callback_impl, &ctx, _1, _2);

      backend->start_run(func);

      ctx.fire_initial_events();

      auto start_ts = std::chrono::steady_clock::now();
      do {
        backend->poll_events();
      } while (ctx.fired > ctx.received.size());
      auto end_ts = std::chrono::steady_clock::now();

      backend->end_run();

      auto diff = end_ts - start_ts;
      auto usec = std::chrono::duration_cast<std::chrono::microseconds>(diff);

      output_console(run, usec.count(), ctx);
      if (output_file.is_open()) {
        output_csv(run, usec.count(), ctx, output_file);
      }

      if (ctx.send_errors || ctx.recv_errors) {
        io_errors = true;
      }
      if (ctx.bytes_received != ctx.received.size()) {
        duplication_errors = true;
      }

      VERBOSE_LOG(opts, "=== End of test run: " << run);
    }

    if (output_file.is_open()) {
      output_file.close();
    }

    if (io_errors) {
      std::cerr << "Benchmark failure due to I/O errors." << std::endl;
      return -1;
    }
    if (duplication_errors) {
      std::cerr << "Benchmark failure due to message duplication errors." << std::endl;
      return -2;
    }
    return 0;
  } catch (packeteer::exception const & ex) {
    std::cerr << ex.what() << std::endl;
    return -3;
  } catch (std::exception const & ex) {
    std::cerr << ex.what() << std::endl;
    return -4;
  } catch (...) {
    std::cerr << "Unknown exception caught, aborting." << std::endl;
    return -5;
  }
}
