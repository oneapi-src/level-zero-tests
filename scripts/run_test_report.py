#!/usr/bin/env python3
# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: MIT

import argparse
from argparse import RawTextHelpFormatter
import os
from typing import Dict,List,TextIO,Iterable
import subprocess
import glob
import re
import sys
import csv
import signal
import level_zero_report_utils

test_plan_generated = []

def IsListableDirPath(path: str):
    try:
        os.listdir(path)
    except Exception as e:
        raise argparse.ArgumentTypeError(path + ' is not a listable directory: ' + str(e))
    return os.path.abspath(path)

def add_to_test_list(test_name: str,
                    binary_and_path: str,
                    test_filter: str,
                    test_feature_tag: str,
                    test_feature: str,
                    requested_types: str,
                    requested_features: str,
                    requested_regex: str,
                    exclude_features: str,
                    exclude_regex: str):
    already_exists = False
    checks_passed = True

    if requested_types and checks_passed:
        test_type_list = requested_types.split(",")
        for test_type in test_type_list:
            if re.search(test_type, test_feature_tag, re.IGNORECASE):
                for i in range(len(test_plan_generated)):
                    if test_name.__eq__(test_plan_generated[i][0]) != 0:
                        already_exists = True
                if not already_exists:
                    checks_passed = True
                    break
                else:
                    checks_passed = False
            else:
                checks_passed = False

    if requested_features and checks_passed:
        feature_list = requested_features.split(",")
        for feature in feature_list:
            if re.search(feature, test_feature, re.IGNORECASE):
                for i in range(len(test_plan_generated)):
                    if test_name.__eq__(test_plan_generated[i][0]) != 0:
                        already_exists = True
                if not already_exists:
                    checks_passed = True
                    break
                else:
                    checks_passed = False
            else:
                checks_passed = False

    if requested_regex and checks_passed:
        if re.search(requested_regex, test_name) or re.search(requested_regex, test_filter):
            for i in range(len(test_plan_generated)):
               if test_name.__eq__(test_plan_generated[i][0]) != 0:
                   already_exists = True
            if not already_exists:
                checks_passed = True
            else:
                checks_passed = False
        else:
            checks_passed = False

    if exclude_features and checks_passed:
        feature_list = exclude_features.split(",")
        for feature in feature_list:
            if re.search(feature, test_feature, re.IGNORECASE):
                checks_passed = False
                break
            else:
                checks_passed = True

    if exclude_regex and checks_passed:
        if re.search(exclude_regex, test_name) or re.search(exclude_regex, test_filter):
            checks_passed = False
        else:
            checks_passed = True

    if checks_passed == True:
        test_plan_generated.append((test_name, test_filter, binary_and_path, test_feature_tag, test_feature))

def run_test_plan(test_plan: [], test_run_timeout: int, fail_log_name: str):
    results = []
    i = 0
    failed = 0
    unsupported = 0
    num_passed = 0
    num_failed = 0
    num_skipped = 0
    fail_log = open(fail_log_name, 'a')
    for i in range(len(test_plan)):
        stdout_log = "_stdout.log"
        stderr_log = "_stderr.log"
        fout = open(stdout_log, 'a+')
        ferr = open(stderr_log, 'a+')
        test_run = subprocess.Popen([test_plan[i][2], test_plan[i][1]], stdout=fout, stderr=ferr, start_new_session=True)
        try:
            failed = 0
            unsupported = 0
            test_run.wait(timeout=test_run_timeout)
            fout.close()
            ferr.close()
            fout = open(stdout_log, 'r')
            ferr = open(stderr_log, 'r')
            output = ferr.readlines() + fout.readlines()
            for line in output:
                if re.search("ZE_RESULT_ERROR_UNSUPPORTED*", line, re.IGNORECASE):
                    unsupported = 1
                    break
                elif re.search("FAILED", line):
                    failed = 1
                    break

            if failed == 1:
                result = (test_plan[i][0], test_plan[i][4], test_plan[i][3], 'FAILED')
                results.append(result)
                num_failed += 1
                fail_log.write(test_plan[i][0] + ' FAILED' + "\n")
                for line in output:
                    fail_log.write(line + "\n")
            elif unsupported == 1:
                result = (test_plan[i][0], test_plan[i][4], test_plan[i][3], 'UNSUPPORTED')
                results.append(result)
                num_skipped += 1
                fail_log.write(test_plan[i][0] + ' UNSUPPORTED' + "\n")
                for line in output:
                    fail_log.write(line + "\n")
            else:
                result = (test_plan[i][0], test_plan[i][4], test_plan[i][3], 'PASSED')
                results.append(result)
                num_passed += 1
            i += 1
            print("-", end = '')
            sys.stdout.flush()
        except subprocess.TimeoutExpired:
            fout.close()
            ferr.close()
            fout = open(stdout_log, 'r')
            ferr = open(stderr_log, 'r')

            result = (test_plan[i][0], test_plan[i][4], test_plan[i][3], 'TIMEOUT')
            results.append(result)
            num_failed += 1
            output = ferr.readlines() + fout.readlines()
            fail_log.write(test_plan[i][0] + ' TIMEOUT' + "\n")
            if output:
                for line in output:
                    fail_log.write(line + "\n")
            os.killpg(os.getpgid(test_run.pid), signal.SIGTERM)
            fout.close()
            ferr.close()
            i += 1
            print("T", end = '')
            sys.stdout.flush()

        fout.close()
        ferr.close()
        os.remove(stdout_log)
        os.remove(stderr_log)
    fail_log.close()
    return results, num_passed, num_failed, num_skipped

def run_test_report(test_plan: [], test_run_timeout: int, log_prefix: str):
    report_log_name = log_prefix + "_results.csv"
    fail_log_name = log_prefix + "_failure_log.txt"
    fail_log = open(fail_log_name, 'w')
    fail_log.close()

    print("Running:", end = '')
    print(len(test_plan), end = '')
    print(" Tests")

    num_passed = 0
    num_failed = 0
    num_skipped = 0
    results = []
    results_header = ("Name", "Feature", "Test Type", "Result")
    results.append(results_header)
    print("<", end = '')
    sys.stdout.flush()

    data = run_test_plan(test_plan, test_run_timeout, fail_log_name)
    results += data[0]
    num_passed += data[1]
    num_failed += data[2]
    num_skipped += data[3]

    print(">\n")
    print("Generating Test Report\n")
    report_results_space = ("")
    total_results_footer = ("", "", "", "", "Total Passed", "Total Failed", "Total Skipped", "Pass Rate")
    total_tests = num_passed + num_failed + num_skipped
    deimal_pass_rate = num_passed/total_tests
    pass_rate = "{:.0%}".format(deimal_pass_rate)
    total_results = ("", "", "", "", num_passed, num_failed, num_skipped, pass_rate)
    results.append(report_results_space)
    results.append(total_results_footer)
    results.append(total_results)

    with open(report_log_name, 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerows(results)

    print("Completed Test Report, see results in " + report_log_name + "\n")
    print("Overall Results| Total Tests: ", end = '')
    print(total_tests, end = '')
    print(" Passed Tests: ", end = '')
    print(num_passed, end = '')
    print(" Failed Tests: ", end = '')
    print(num_failed, end = '')
    print(" Skipped Tests: ", end = '')
    print(num_skipped, end = '')
    print(" Pass Rate: ", end = '')
    print(pass_rate)
    print("\n")
    if num_failed > 0:
        print("Tests Failed, see verbose error results in " + fail_log_name)
        return -1
    elif num_skipped > 0:
        print("Some Tests Were skipped as unsupported on this system, see verbose results in " + fail_log_name)
    else:
        return 0

def generate_test_case(binary_and_path: str,
                    suite_name: str,
                    test_binary: str,
                    line: str,
                    requested_types: str,
                    requested_features: str,
                    requested_regex: str,
                    exclude_features: str,
                    exclude_regex: str
                    ):
        test_name, case_name, test_section = level_zero_report_utils.create_test_name(suite_name, test_binary, line)
        if test_name.find("DISABLED") != -1:
            return
        test_filter = "--gtest_filter=*" + case_name + "*"
        test_feature, test_section_by_feature = level_zero_report_utils.assign_test_feature(test_binary, test_name)
        if (test_section == "None"):
            test_section= test_section_by_feature
        test_feature_tag = level_zero_report_utils.assign_test_feature_tag(test_feature, test_name, test_section)
        add_to_test_list(test_name, binary_and_path, test_filter, test_feature_tag, test_feature, requested_types, requested_features, requested_regex, exclude_features, exclude_regex)

def generate_test_items_for_binary(binary_dir: str,
                                test_binary: str,
                                requested_types: str,
                                requested_features: str,
                                requested_regex: str,
                                exclude_features: str,
                                exclude_regex: str
                                ):
    test_binary_path = os.path.join(binary_dir, test_binary)
    if (test_binary.find("test") != -1):
        popen = subprocess.Popen([test_binary_path, '--gtest_list_tests', '--logging-level=error'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output = popen.stdout.read()
        output = output.decode('utf-8').splitlines()

        # parameterized cases are reduced to a single case for all parameterizations
        current_suite = None
        for line in output:
            if line[0] != ' ':  # test suite
                current_suite = line.split('.')[0]
            else:  # test case
                generate_test_case(test_binary_path, current_suite, test_binary, line, requested_types, requested_features, requested_regex, exclude_features, exclude_regex)

def generate_test_cases_from_binaries(binary_dir: str,
                                    requested_types: str,
                                    requested_features: str,
                                    requested_regex: str,
                                    exclude_features: str,
                                    exclude_regex: str
                                    ):
    test_binaries = [
      filename for filename in os.listdir(binary_dir)
      if os.access(os.path.join(binary_dir, filename), os.X_OK) and
      os.path.isfile(os.path.join(binary_dir, filename))]

    for binary in test_binaries:
        generate_test_items_for_binary(binary_dir, binary, requested_types, requested_features, requested_regex, exclude_features, exclude_regex)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='''Standalone Level Zero Test Runner for Reports''',
    epilog="""example:\n
        python3 run_all_tests.py \n
            --binary_dir /build/out/conformance_tests/
            --run_test_types \"basic\"
            --run_test_features \"barrier\"
            --run_test_regex \"GivenEvent*\"
            --exclude_features \"image\"
            --exclude_regex \"events*\"
            --test_run_timeout 15
            --log_prefix \"level_zero_tests_1234\"\n""", formatter_class=RawTextHelpFormatter)
    parser.add_argument('--binary_dir', type = IsListableDirPath, help = 'Directory containing gtest binaries and SPVs.', required = True)
    parser.add_argument('--run_test_types', type = str, help = 'List of Test types to include Comma Separated: basic,advanced,discrete,tools,negative,stress,all NOTE:all sets all types', default = "basic")
    parser.add_argument('--run_test_features', type = str, help = 'List of Test Features to include Comma Separated: barrier,...', default = None)
    parser.add_argument('--run_test_regex', type = str, help = 'Regular Expression to filter tests that match either in the name or filter: GivenBarrier*', default = None)
    parser.add_argument('--exclude_features', type = str, help = 'List of Test Features to exclude Comma Separated: barrier,...', default = None)
    parser.add_argument('--exclude_regex', type = str, help = 'Regular Expression to exclude tests that match either in the name or filter: GivenBarrier*', default = None)
    parser.add_argument('--test_run_timeout', type = int, help = 'Adjust the timeout for test binary execution, this is for each call to the binary with the filter', default = 15)
    parser.add_argument('--log_prefix', type = str, help = 'Change the prefix name for the results such that the output is <prefix>_results.csv & <prefix>_failure_log.txt', default = "level_zero_tests")
    args = parser.parse_args()

    run_test_types = args.run_test_types

    if re.search(run_test_types, "all", re.IGNORECASE):
        run_test_types = "basic,advanced,discrete,tools,negative,stress"

    run_test_features = args.run_test_features
    run_test_regex = args.run_test_regex
    test_run_timeout = args.test_run_timeout
    log_prefix = args.log_prefix
    exclude_features = args.exclude_features
    exclude_regex = args.exclude_regex
    exit_code = -1

    print("Level Zero Test Report Generator\n")
    print("Executing Tests which match Test Type(s):" + run_test_types + " ", end = '')
    if run_test_features:
        print(" Test Feature(s):" + run_test_features, end = '')
    if run_test_regex:
        print(" Test Regex:" + run_test_regex, end = '')
    if exclude_features:
        print(" Excluding Feature(s):" + exclude_features, end = '')
    if exclude_regex:
        print(" Excluding Regex:" + exclude_regex, end = '')
    print("\n")

    generate_test_cases_from_binaries(
      binary_dir = args.binary_dir,
      requested_types = run_test_types,
      requested_features = run_test_features,
      requested_regex = run_test_regex,
      exclude_features = exclude_features,
      exclude_regex = exclude_regex
      )
    print("Generated Test List\n")

    if len(test_plan_generated) > 0:
        exit_code = run_test_report(test_plan = test_plan_generated, test_run_timeout = test_run_timeout, log_prefix = log_prefix)
    else:
        print("Test Filters set are invalid, no tests that match all requirements.")
    exit(exit_code)
