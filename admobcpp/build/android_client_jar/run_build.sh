#!/bin/bash

set -e

# Point this to a directory containing an Android-compatible javac
JAVAC_HOME=/Library/Java/JavaVirtualMachines/jdk1.7.0_75.jdk/Contents/Home/bin

# Compile the java files, referencing the current google play services jar.
find ../../src/android_java -name "*.java" | xargs ${JAVAC_HOME}/javac -cp ../../libs/google-play-services.jar:. -d classes

# Jar it up, place in libs directory
cd classes
find . -name "*.class" | xargs jar cvf ../../../libs/gma_cpp_client.jar
cd ..
