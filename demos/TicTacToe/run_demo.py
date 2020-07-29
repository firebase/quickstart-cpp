#!/usr/bin/python
# coding=utf-8

# Copyright 2020 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
import os
import subprocess
import sys


def logger_setup():
    # The root logger of the hierarchy.
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)

    # Add a StreamHandler to log to stdout.
    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        "%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    stream_handler.setFormatter(formatter)
    logger.addHandler(stream_handler)

    return logger


def log_run(dir, logger, cmd):
    # Logs the command.
    logger.info(cmd)
    # Runs the command.
    subprocess.call(cmd, cwd=dir, shell=True)


def main():
    """The main function."""
    # The run_demo.py script directory.
    ROOT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))

    # Sets up the logging format and handler.
    logger = logger_setup()

    # Directory paths.
    game_name = "tic_tac_toe_demo"
    executable_dir = os.path.join(ROOT_DIRECTORY, game_name, "build", "bin",
                                  game_name, "Debug")

    # Checks whether the demo was set up properly.
    if not os.path.isdir(executable_dir):
        logger.error(
            "Demo setup incomplete. Did you run the setup script first?")
        exit()

    # Checks whether the google-services.json exists in the debug directory.
    if not os.path.isfile(os.path.join(executable_dir,
                                       "google-services.json")):
        logger.error(
            "google-services.json file is missing in {} directory.".format(
                executable_dir))
        exit()

    # Checks whether the executable exists in the debug directory.
    if os.path.isfile(os.path.join(executable_dir,
                                   "{}.exe".format(game_name))):
        logger.info("Lanching the demo...")
        # Runs the tic-tac-toe executable.
        log_run(executable_dir, logger, '{}.exe'.format(game_name))
    else:
        logger.error("Game executable does not exist.")
        exit()


# Check to see if this script is being called directly.
if __name__ == "__main__":
    exit(main())
