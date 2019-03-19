// Copyright 2019 Lawrence Livermore National Security, LLC and other Metall Project Developers.
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#ifndef METALL_BENCH_BFS_DRIVER_HPP
#define METALL_BENCH_BFS_DRIVER_HPP

#include <string>
#include <unistd.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "kernel.hpp"
#include "../utility/time.hpp"
#include "../utility/memory.hpp"

namespace bfs_bench {

// ---------------------------------------- //
// Option
// ---------------------------------------- //
template <typename _vertex_id_type>
struct bench_options {
  using vertex_id_type = _vertex_id_type;

  std::string graph_file_name;
  std::string graph_key_name{"adj_list"};
  vertex_id_type root_vertex_id{0};
  vertex_id_type max_vertex_id{0};
};

template <typename vertex_id_type>
bool parse_options(int argc, char **argv, bench_options<vertex_id_type> *option) {
  int p;
  while ((p = ::getopt(argc, argv, "g:k:r:m:")) != -1) {
    switch (p) {
      case 'g':option->graph_file_name = optarg;
        break;

      case 'k':option->graph_key_name = optarg;
        break;

      case 'r':option->root_vertex_id = static_cast<vertex_id_type>(std::stoll(optarg));
        break;

      case 'm':option->max_vertex_id = static_cast<vertex_id_type>(std::stoll(optarg));
        break;

      default:std::cerr << "Invalid option" << std::endl;
        return false;
    }
  }

  if (option->graph_file_name.empty()) {
    std::cerr << "graph_file_name is required" << std::endl;
    return false;
  }

  if (option->max_vertex_id < 1) {
    std::cerr << "Invalid max_vertex_id " << option->max_vertex_id << std::endl;
    return false;
  }

  std::cout << "graph_file_name: " << option->graph_file_name
            << "\ngraph_key_name: " << option->graph_key_name
            << "\nroot_vertex_id: " << option->root_vertex_id
            << "\nmax_vertex_id: " << option->max_vertex_id << std::endl;

  return true;
}

// ---------------------------------------- //
// Find max ID
// ---------------------------------------- //
template <typename graph_type>
typename graph_type::key_type find_max_id(const graph_type &graph) {
  typename graph_type::key_type max_id = std::numeric_limits<typename graph_type::key_type>::min();

  for (auto source_itr = graph.keys_begin(), source_end = graph.keys_end(); source_itr != source_end; ++source_itr) {
    const auto &source = source_itr->first;
    max_id = std::max(max_id, source);
  }

  return max_id;
}

// ---------------------------------------- //
// Find root
// ---------------------------------------- //
template <typename graph_type>
typename graph_type::key_type find_root(const graph_type &graph) {
  for (auto source_itr = graph.keys_begin(), source_end = graph.keys_end(); source_itr != source_end; ++source_itr) {
    const auto &source = source_itr->first;
    if (graph.num_values(source) > 0) {
      return source;
    }
  }
  std::cerr << "Cannot find a vertex that has an edge" << std::endl;
  std::abort();
}

// ---------------------------------------- //
// Utility
// ---------------------------------------- //
void print_current_num_page_faults() {
  const auto page_faults = utility::get_num_page_faults();
  std::cout << "#of page faults (minflt majflt) " << page_faults.first << " " << page_faults.second << std::endl;
}


/// \brief Print out Open MP's configuration
void print_omp_configuration() {
#ifdef _OPENMP
#pragma omp parallel
  {
    if (::omp_get_thread_num() == 0)
      std::cout << "Run with " << ::omp_get_num_threads() << " threads" << std::endl;
  }
  omp_sched_t kind;
  int chunk_size;
  ::omp_get_schedule(&kind, &chunk_size);
  std::cout << "kind " << utility::omp_schedule_kind_name(kind)
            << ", chunk_size " << chunk_size << std::endl;
#else
  std::cout << "Run with a single thread" << std::endl;
#endif
}

// ---------------------------------------- //
// Benchmark driver
// ---------------------------------------- //
template <typename graph_type, typename vertex_id_type>
void run_bench(const graph_type &graph, const bench_options<vertex_id_type> &option) {
  std::cout << "\nBFS kernel" << std::endl;
  print_current_num_page_faults();

  vertex_id_type new_root;
  {
    new_root = graph.num_values(option.root_vertex_id) ? option.root_vertex_id : find_root(graph);
    std::cout << "\nBFS root\t" << new_root << std::endl;
    print_current_num_page_faults();
  }

  vertex_id_type max_id = option.max_vertex_id;
  if (max_id == 0) {
    std::cout << "\nFind the max vertex ID" << std::endl;
    const auto start = utility::elapsed_time_sec();
    max_id = find_max_id(graph);
    const auto elapsed_time = utility::elapsed_time_sec(start);
    std::cout << "Finished finding the max ID (s)\t" << elapsed_time << std::endl;
    print_current_num_page_faults();
  }

  bfs_data data;
  {
    std::cout << "\nInitialize bfs" << std::endl;
    const auto start = utility::elapsed_time_sec();
    initialize(max_id, new_root, &data);
    const auto elapsed_time = utility::elapsed_time_sec(start);
    std::cout << "Finished initialization (s)\t" << elapsed_time << std::endl;
    print_current_num_page_faults();
  }

  {
    std::cout << "\nStart BFS" << std::endl;
    print_omp_configuration();
    print_current_num_page_faults();
    const auto start = utility::elapsed_time_sec();
    kernel(graph, &data);
    const auto elapsed_time = utility::elapsed_time_sec(start);
    std::cout << "Finished BFS (s)\t" << elapsed_time << std::endl;
    print_current_num_page_faults();
  }

  count_level(data);
}

} // namespace bfs_bench

#endif //METALL_BENCH_BFS_DRIVER_HPP
