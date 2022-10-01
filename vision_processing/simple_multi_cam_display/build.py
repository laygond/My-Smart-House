# Script to delete old build folder and build it from scratch
# by Bryan Laygond

import sys 
import os
import shutil

def rm_build():
  for elem in os.listdir():
    if elem=='build' and os.path.isdir(elem):
      shutil.rmtree(elem)
      break

# Change directory to where this script is
base_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(base_dir)

# Verify Build instructions
if 'CMakeLists.txt' in os.listdir():
  
  unix_OS = ['linux','darwin']
  win_OS = ['win32','cygwin']
  # Remove and Build according to OS
  if sys.platform  in unix_OS:
    rm_build()
    os.mkdir('build')
    os.chdir('build')
    os.system('cmake ..')
    os.system('make')
  elif sys.platform in win_OS:
    rm_build()
    os.mkdir('build')
    os.chdir('build')
    os.system('cmake -G \"MinGW Makefiles\" ..')
    os.system('mingw32-make')
  else:
    print('[INFO]: This OS is not registered. Unable to create Build Files.')

else:
  print('[INFO]: No CMakeLists.txt found. Unable to create Build Files.')


