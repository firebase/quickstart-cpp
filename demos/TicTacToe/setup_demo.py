#!/usr/bin/python
# coding=utf-8

# Copyright 2020 Google Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


import fileinput
import glob
import logging
import os
import subprocess
import sys


# The setup_demo.py script directory.
ROOT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))

def main():
  """The main function."""
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

  # Game directories. 
  game_name = "tic_tac_toe_demo"
  game_resources_dir = os.path.join(ROOT_DIRECTORY, "game_resources")
  game_files_dir = os.path.join(ROOT_DIRECTORY, game_name);
  debug_exe_path = os.path.join(game_files_dir,"build","bin",game_name, "Debug")

  # Creating the cocos2d-x project. 
  os.system("cocos new {0} -p com.DemoApp.{0} -l cpp -d .".format(game_name))
  # Removing the default cocos2d-x project files.
  os.system("rm -r {0}/Classes {0}/Resources {0}/CMakeLists.txt".format(game_files_dir))
  # Copies the google-services.json file into the correct directory to run the executable. 
  os.system("cp google_services/google-services.json {}".format(os.path.join(game_resources_dir,"build","bin", game_name, "Debug")))
  # Copies the tic-tac-toe game files into the cocos2d-x project files.
  os.system("cp {} {} -TRv".format(game_resources_dir,game_files_dir))
  # Changes directory into the build directory.
  os.chdir(os.path.join(game_files_dir,"build"))
  # Runs cmake with the Visual Studio 2019 as the generator and windows as the target.
  os.system('cmake .. -G "Visual Studio 16 2019" -A Win32')
  os.system("cmake --build .")
  # Changes direcotyr to the executable.
  os.chdir(debug_exe_path)
  # Runs the tic-tac-toe executable.
  subprocess.call('{}.exe'.format(game_name))

# Check to see if this script is being called directly.
if __name__ == "__main__":
  exit(main())