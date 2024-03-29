#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess
from pathlib import Path


def check_project_root(project_root, spec_test_dir):
    if not spec_test_dir.is_dir():
        print("Error: test/testsuite not found.")
        print("This script must be run from the project root folder.")
        print("Please change to the project root directory and try again.")
        sys.exit(1)


def check_wast2json():
    if not shutil.which("wast2json"):
        print("Error: wast2json could not be found. Make sure it is installed and in your PATH.")
        sys.exit(1)


def create_output_dir(output_dir):
    output_dir.mkdir(parents=True, exist_ok=True)


def process_spec_tests(spec_test_dir, output_dir):
    spec_test_dir = Path(spec_test_dir)
    output_dir = Path(output_dir)

    wast_files = list(spec_test_dir.glob("*.wast"))

    if not wast_files:
        print("Error: No '.wast' files found in the specified directory.")
        print("       Make sure to download the spec tests by updating the submodule with:")
        print("           $git submodule init")
        print("           $git submodule update")
        sys.exit(1)

    for test_file in wast_files:
        print(f"Processing: {test_file.name}")
        raw_test_file_path = r'{}'.format(test_file)
        output_file = output_dir / f"{test_file.stem}.json"
        returncode = subprocess.call(["wast2json", "-o", str(output_file), raw_test_file_path])

        if returncode != 0:
            print(f"Error: Failed to process '{test_file}'")
            sys.exit(1)

    print(f"All spec tests have been successfully processed and saved to '{output_dir}'.")

def main():
    project_root = Path.cwd()
    spec_test_dir = project_root / "test" / "testsuite"
    output_dir = project_root / "build" / "testsuite"
    check_project_root(project_root, spec_test_dir)
    check_wast2json()
    create_output_dir(output_dir)
    os.chdir(output_dir)
    process_spec_tests(spec_test_dir, output_dir)
    os.chdir(project_root)


if __name__ == "__main__":
    main()
