# ReaderWriterLock
A reader writer lock, that allows you to query count of waiting reader and writer threads on the lock.

## Setup instructions
**Install dependencies :**
 * This project does not have any dependencies.

**Download source code :**
 * `git clone https://github.com/RohanVDvivedi/ReaderWriterLock.git`

**Build from source :**
 * `cd ReaderWriterLock`
 * `make clean all`

**Install from the build :**
 * `sudo make install`
 * ***Once you have installed from source, you may discard the build by*** `make clean`

## Using The library
 * add `-lrwlock -lpthread` linker flag, while compiling your application
 * do not forget to include appropriate public api headers as and when needed. this includes
   * `#include<rwlock.h>`

## Instructions for uninstalling library

**Uninstall :**
 * `cd ReaderWriterLock`
 * `sudo make uninstall`
