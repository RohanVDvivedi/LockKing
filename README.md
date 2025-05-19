# LockKing
A Lock library that is a king of it's kind.

It provides,

A reader writer lock implementaton (rwlock) that allows
 * taking locks READ_PREFERRING-ly or WRITE_PREFERRING-ly
 * taking locks BLOCKING-ly or NON_BLOCKING-ly or with a timeout_in_microseconds
 * It allows you to downgrade writer lock to reader lock and upgrade reader lock to writer lock (with safety from deadlocks arising out of concurrent upgraders)
 * It allows you to have an external lock allowing you to build complex functionalities aroung this lock (see my projects Bufferpool and WALe)

## Setup instructions
**Install dependencies :**
  * [PosixUtils](https://github.com/RohanVDvivedi/PosixUtils)

**Download source code :**
 * `git clone https://github.com/RohanVDvivedi/ReaderWriterLock.git`

**Build from source :**
 * `cd ReaderWriterLock`
 * `make clean all`

**Install from the build :**
 * `sudo make install`
 * ***Once you have installed from source, you may discard the build by*** `make clean`

## Using The library
 * add `-llockking -lpthread` linker flag, while compiling your application
 * do not forget to include appropriate public api headers as and when needed. this includes
   * `#include<rwlock/rwlock.h>`

## Instructions for uninstalling library

**Uninstall :**
 * `cd ReaderWriterLock`
 * `sudo make uninstall`
