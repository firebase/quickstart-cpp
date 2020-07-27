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

import fileinput
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

def modify_proj_file(dir):
  f = fileinput.FileInput(files = [os.path.join(dir,"main.cpp")], inplace = True)
  for line in f:
    print line.replace("AppDelegate.h", "app_delegate.h"),

def main():
  """The main function."""
  # The setup_demo.py script directory.
  ROOT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
  
  # Sets up the logging format and handler.
  logger = logger_setup()
 
  # Directory paths.
  game_name = "tic_tac_toe_demo"
  game_resources_dir = os.path.join(ROOT_DIRECTORY, "game_resources")
  game_files_dir = os.path.join(ROOT_DIRECTORY, game_name);
  windows_proj_dir = os.path.join(game_files_dir,"proj.win32")
  mac_proj_dir = os.path.join(game_files_dir,"proj.ios_mac","mac")
  linux_proj_dir = os.path.join(game_files_dir,"proj.linux")
  build_dir = os.path.join(game_files_dir,"build")
  executable_dir = os.path.join(build_dir,"bin",game_name,"Debug")

  # Creating the cocos2d-x project. 
  log_run(ROOT_DIRECTORY, logger,"cocos new {0} -p com.DemoApp.{0} -l cpp -d .".format(game_name))

  # Removing the default cocos2d-x project files.
  log_run(ROOT_DIRECTORY, logger,"rm -r {0}/Classes {0}/Resources {0}/CMakeLists.txt".format(game_files_dir) )

  # Copies the google-services.json file into the correct directory to run the executable. 
  log_run(ROOT_DIRECTORY, logger,"cp google_services/google-services.json {}".format(os.path.join(game_resources_dir,"build","bin", game_name, "Debug")))
 
  # Copies the tic-tac-toe game files into the cocos2d-x project files.
  log_run(ROOT_DIRECTORY, logger, "cp {} {} -TRv".format(game_resources_dir,game_files_dir))

  # Changes the windows project main.cpp to include the new app_delegate header.
  modify_proj_file(windows_proj_dir)

  # Changes directory into the build directory.
  log_run(build_dir, logger, 'cmake .. -G "Visual Studio 16 2019" -A Win32')
 
  # Runs cmake with the Visual Studio 2019 as the generator and windows as the target.
  log_run(build_dir, logger,"cmake --build .")
 
  # Runs the tic-tac-toe executable.
  log_run(executable_dir, logger,'{}.exe'.format(game_name))

# Check to see if this script is being called directly.
if __name__ == "__main__":
  exit(main())
