# ReaderWriterLock
A reader writer lock that allows
 * taking locks READ_PREFERRING or WRITE_PREFERRING
 * taking locks BLOCKING-ly or NON_BLOCKING-ly
 * It allows you to downgrade writer lock to reader lock and upgrade reader lock to writer lock (second of which may fail).
 * It allows you to have an external lock allowing you to build complex functionalities aroung this lock (see my projects Bufferpool and WALe).

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
