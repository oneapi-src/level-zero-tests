/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "workload.hpp"

namespace compute_api_bench {

#define WARMUP_ITERATIONS 2

Workload::Workload() {}

void Workload::run(unsigned int iterations) {
  try {
    Timer timer;

    // Warm-up
    for (unsigned int i = 0; i < WARMUP_ITERATIONS; ++i) {
      create_device();
      build_program();
      create_buffers();
      create_cmdlist();
      execute_work();
      cleanup();
    }

    for (unsigned int i = 0; i < iterations; ++i) {
      std::cout << "\r" << workload_api << " " << workload_name
                << " - iteration: " << i + 1 << std::flush;
      timer.start();
      create_device();
      result[Stages::CREATE_DEVICE].times.push_back(timer.elapsed_time());

      build_program();
      result[Stages::BUILD_PROGRAM].times.push_back(timer.elapsed_time());

      create_buffers();
      create_cmdlist();
      result[Stages::CREATE_BUFFERS_CMDLIST].times.push_back(
          timer.elapsed_time());

      execute_work();
      result[Stages::EXECUTE_WORK].times.push_back(timer.elapsed_time());

      if (verify_results() == false) {
        cleanup();
        std::cout << "\r" << std::flush;
        std::cout << "Verification failed! Aborting the test..." << std::endl;
        exit(0);
      }

      cleanup();

      std::cout << "\r" << std::flush;
    }

    calculate_results();
  } catch (const std::exception &e) {
    std::cout << "Exception occured: " << e.what();
    exit(0);
  }
}

void Workload::calculate_results() {

  for (unsigned int i = 0; i < Stages::COUNT; ++i) {
    result[i].time_mean =
        std::accumulate(result[i].times.begin(), result[i].times.end(), 0.0f) /
        result[i].times.size();

    double error = 0.0;
    for (double time : result[i].times) {
      error += pow(time - result[i].time_mean, 2);
    }

    result[i].time_standard_deviation = sqrt(error / result[i].times.size());

    std::sort(result[i].times.begin(), result[i].times.end());
    result[i].time_min = result[i].times.front();
    result[i].time_max = result[i].times.back();

    unsigned int middleIdx = result[i].times.size() / 2;

    if (result[i].times.size() % 2) {
      result[i].time_median = result[i].times[middleIdx];
    } else {
      result[i].time_median =
          (result[i].times[middleIdx - 1] + result[i].times[middleIdx]) / 2;
    }
  }
}

void Workload::print_total_mean_time() {
  std::cout.precision(4);
  double total_time = 0;
  for (unsigned int i = 0; i < Stages::COUNT; ++i) {
    total_time += result[i].time_mean;
  }

  std::string tmp = workload_api + " " + workload_name + " overall mean time: ";
  std::cout << std::left << std::setw(47) << tmp << std::right << std::setw(6)
            << total_time * 1000.0f << " ms" << std::endl;
}

void Workload::print_stage_mean_sd(unsigned int stage, std::string &csv_string,
                                   bool colored, bool useMedian) {
  double sd_percent =
      result[stage].time_standard_deviation / result[stage].time_mean * 100.0f;

  std::string color = "", reset_color = "";
  if (colored) {
    color = intense_white;
    reset_color = reset;

    if (sd_percent > 10.0f) {
      color = light_red;
    } else if (sd_percent > 5.0f) {
      color = yellow;
    } else
      color = green;
  }

  csv_string += std::to_string(result[stage].time_mean * 1000.0f) + "," +
                std::to_string(sd_percent / 100.0f) + ",";
  if (useMedian) {
    std::cout << std::setw(9) << result[stage].time_median * 1000.0f;
  } else {
    std::cout << std::setw(9) << result[stage].time_mean * 1000.0f;
  }
  std::cout << " (SD: " << color << std::setw(3) << (int)sd_percent
            << reset_color << "%)  |  ";
}

void Workload::print_apis(std::string api, std::string &csv_string,
                          bool colored) {
  std::string color = "", reset_color = "";
  if (colored) {
    color = intense_white;
    reset_color = reset;
  }

  if (api == "all") {
    csv_string += ",OpenCL Mean,OpenCL SD,Level-Zero Mean,Level-Zero SD\n";
    std::cout << std::setw(25) << " "
              << "  |  ";
    std::cout << color << std::setw(20) << "OpenCL" << reset_color << "  |  ";
    std::cout << color << std::setw(20) << "Level-Zero" << reset_color
              << "  |  ";
    std::cout << std::endl;
  } else {
    csv_string += ", " + api + " Mean," + api + " SD\n";
    std::cout << std::setw(25) << " "
              << "  |  " << color << std::setw(20) << api << reset_color
              << "  |  " << std::endl;
  }
}

} // namespace compute_api_bench
