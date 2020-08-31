/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common/utils.hpp"
#include "common/workload.hpp"
#include "opencl/ocl_simpleadd.hpp"
#include "opencl/ocl_mandelbrot.hpp"
#include "opencl/ocl_sobel.hpp"
#include "opencl/ocl_blackscholes.hpp"
#include "opencl/ocl_blackscholes.cpp"
#include "level-zero/ze_simpleadd.hpp"
#include "level-zero/ze_mandelbrot.hpp"
#include "level-zero/ze_sobel.hpp"
#include "level-zero/ze_blackscholes.hpp"
#include "level-zero/ze_blackscholes.cpp"

using namespace compute_api_bench;

#define NUM_ITERATIONS 30
#define SOBEL_ITERATIONS 200
#define MANDELBROT_ITERATIONS 50
#define BLACKSCHOLES_ITERATIONS 50
#define SIMPLEADD_NUM_ELEMENTS 100000
#define MANDELBROT_WIDTH 1024
#define MANDELBROT_HEIGHT 1024
#define BLACKSCHOLES_NUM_OPTIONS 1024 * 1024

void print_help() {
  printf(R"===(
ze_cabe is a benchmark designed to compare the performance of level-zero and opencl.

Parameters:
 -api <api> - Valid values: opencl, level-zero, all. The default is all. 
 -scenario <scenario> - Valid values: simpleadd, mandelbrot, sobel, blackscholesfp32,
                        blackscholesfp64, all. The default is all.
 -iterations <X> - X is a value between 1..200. The default is 30.
 -median - uses median instead of default mean for reporting detailed results.
 -csv <filename> - saves results to a filename file in csv format. 
 -color - presents SDs in color (does not work in Windows cmd).

Usage examples:
 ze_cabe -api opencl
 ze_cabe -api level-zero -scenario sobel -iterations 10 -csv out.csv -color

)===");
}

int main(int argc, char *argv[]) {

  std::vector<std::string> valid_apis = {"opencl", "level-zero", "all"};
  std::vector<std::string> valid_scenarios = {
      "simpleadd",        "mandelbrot",       "sobel",
      "blackscholesfp32", "blackscholesfp64", "all"};
  std::vector<Workload *> ocl_workloads;
  std::vector<Workload *> levelzero_workloads;
  std::string api = "all";
  std::string scenario = "all";
  unsigned int iterations = NUM_ITERATIONS;
  unsigned int number_of_scenarios = valid_scenarios.size() - 1;
  bool write_csv = false;
  std::string csv_filename;
  level_zero_tests::ImageBMP8Bit image("lena512.bmp");
  bool colored = false;
#if defined __linux__
  colored = true;
#endif
  bool useMedian = false;

  for (uint32_t argIndex = 1; argIndex < argc; argIndex++) {
    if (!strcmp(argv[argIndex], "-h") || !strcmp(argv[argIndex], "-help")) {
      print_help();
      exit(0);
    } else if (!strcmp(argv[argIndex], "-api") && (argIndex + 1 < argc)) {
      api = argv[argIndex + 1];
      if (std::find(valid_apis.begin(), valid_apis.end(), scenario) ==
          valid_apis.end()) {
        std::cout << "Invalid api!" << std::endl;
        exit(0);
      }
      argIndex++;
    } else if (!strcmp(argv[argIndex], "-scenario") && (argIndex + 1 < argc)) {
      scenario = argv[argIndex + 1];
      if (scenario != "all")
        number_of_scenarios = 1;
      if (std::find(valid_scenarios.begin(), valid_scenarios.end(), scenario) ==
          valid_scenarios.end()) {
        std::cout << "Invalid scenario!" << std::endl;
        exit(0);
      }
      argIndex++;
    } else if (!strcmp(argv[argIndex], "-iterations") &&
               (argIndex + 1 < argc)) {
      iterations = std::stoi(argv[argIndex + 1]);
      if (iterations < 1 && iterations > 200) {
        std::cout << "Invalid number of iterations!" << std::endl;
        exit(0);
      }
      argIndex++;
    } else if (!strcmp(argv[argIndex], "-csv") && (argIndex + 1 < argc)) {
      write_csv = true;
      csv_filename = argv[argIndex + 1];
      argIndex++;
    } else if (!strcmp(argv[argIndex], "-color")) {
      colored = true;
    } else if (!strcmp(argv[argIndex], "-median")) {
      useMedian = true;
    } else {
      std::cout << "Invalid parameters!" << std::endl;
      exit(0);
    }
  }

  std::cout << "Selected api: " << api << ", scenario: " << scenario
            << ", number of iterations: " << iterations << ", ";
  if (useMedian)
    std::cout << "using median for reporting detailed results";
  else
    std::cout << "using mean for reporting detailed results";
  std::cout << std::endl << std::endl;

  BlackScholesData<float> bs_io_data_fp32(BLACKSCHOLES_NUM_OPTIONS);
  BlackScholesData<double> bs_io_data_fp64(BLACKSCHOLES_NUM_OPTIONS);

  if (scenario == "blackscholesfp32" || scenario == "all") {
    bs_io_data_fp32.generate_data();
  }
  if (scenario == "blackscholesfp64" || scenario == "all") {
    bs_io_data_fp64.generate_data();
  }

  OCLSimpleAdd oclSimpleAdd(SIMPLEADD_NUM_ELEMENTS);
  OCLMandelbrot oclMandelbrot(MANDELBROT_WIDTH, MANDELBROT_HEIGHT,
                              MANDELBROT_ITERATIONS);
  OCLSobel oclSobel(image, SOBEL_ITERATIONS);
  OCLBlackScholes<float> oclBlackScholesFP32(&bs_io_data_fp32,
                                             BLACKSCHOLES_ITERATIONS);
  OCLBlackScholes<double> oclBlackScholesFP64(&bs_io_data_fp64,
                                              BLACKSCHOLES_ITERATIONS);

  if (api == "opencl" || api == "all") {
    std::cout << "Testing OpenCL" << std::endl;
    if (scenario == "simpleadd" || scenario == "all") {
      ocl_workloads.push_back(&oclSimpleAdd);
    }
    if (scenario == "mandelbrot" || scenario == "all") {
      ocl_workloads.push_back(&oclMandelbrot);
    }
    if (scenario == "sobel" || scenario == "all") {
      ocl_workloads.push_back(&oclSobel);
    }
    if (scenario == "blackscholesfp32" || scenario == "all") {
      ocl_workloads.push_back(&oclBlackScholesFP32);
    }
    if (scenario == "blackscholesfp64" || scenario == "all") {
      ocl_workloads.push_back(&oclBlackScholesFP64);
    }
    for (auto workload : ocl_workloads) {
      workload->run(iterations);
      workload->print_total_mean_time();
    }
  }

  ZeSimpleAdd zeSimpleAdd(SIMPLEADD_NUM_ELEMENTS);
  ZeMandelbrot zeMandelbrot(MANDELBROT_WIDTH, MANDELBROT_HEIGHT,
                            MANDELBROT_ITERATIONS);
  ZeSobel zeSobel(image, SOBEL_ITERATIONS);
  ZeBlackScholes<float> zeBlackScholesFP32(&bs_io_data_fp32,
                                           BLACKSCHOLES_ITERATIONS);
  ZeBlackScholes<double> zeBlackScholesFP64(&bs_io_data_fp64,
                                            BLACKSCHOLES_ITERATIONS);

  if (api == "level-zero" || api == "all") {
    std::cout << "Testing Level-Zero" << std::endl;
    if (scenario == "simpleadd" || scenario == "all") {
      levelzero_workloads.push_back(&zeSimpleAdd);
    }
    if (scenario == "mandelbrot" || scenario == "all") {
      levelzero_workloads.push_back(&zeMandelbrot);
    }
    if (scenario == "sobel" || scenario == "all") {
      levelzero_workloads.push_back(&zeSobel);
    }
    if (scenario == "blackscholesfp32" || scenario == "all") {
      levelzero_workloads.push_back(&zeBlackScholesFP32);
    }
    if (scenario == "blackscholesfp64" || scenario == "all") {
      levelzero_workloads.push_back(&zeBlackScholesFP64);
    }
    for (auto workload : levelzero_workloads) {
      workload->run(iterations);
      workload->print_total_mean_time();
    }
  }

  std::cout << std::endl;
  std::string csv_string = "";

  Workload::print_apis(api, csv_string, colored);
  std::string workload_name;

  std::string color = "", reset_color = "";
  if (colored) {
    color = intense_white;
    reset_color = reset;
  }

  for (int i = 0; i < number_of_scenarios; ++i) {

    if (number_of_scenarios == 1) {
      std::cout << color << std::left << std::setw(25) << scenario
                << reset_color << "  |  " << std::setw(20) << " "
                << "  |  ";
      csv_string += scenario;
    } else {
      std::cout << color << std::left << std::setw(25) << valid_scenarios[i]
                << reset_color << "  |  " << std::setw(20) << " "
                << "  |  ";
      csv_string += valid_scenarios[i] + ",";
    }

    if (api == "all") {
      std::cout << std::setw(20) << " "
                << "  |  ";
    }

    std::cout << std::endl;
    csv_string += "\n";

    for (unsigned int j = 0; j < Stages::COUNT; ++j) {
      std::cout << std::left << std::setw(25) << StagesList[j] << std::right
                << "  |  ";
      csv_string += StagesList[j] + ",";
      if (api == "opencl" || api == "all") {
        ocl_workloads[i]->print_stage_mean_sd(j, csv_string, colored,
                                              useMedian);
      }
      if (api == "level-zero" || api == "all") {
        levelzero_workloads[i]->print_stage_mean_sd(j, csv_string, colored,
                                                    useMedian);
      }
      std::cout << std::endl;
      csv_string += "\n";
    }
  }

  if (write_csv) {
    save_csv(csv_string, csv_filename);
  }

  return 0;
}
