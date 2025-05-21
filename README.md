# LockKing
A Lock library that is a king of it's kind.

It provides,

1. A reader writer lock implementaton (rwlock) that allows
  * Taking locks READ_PREFERRING-ly or WRITE_PREFERRING-ly
  * Taking locks BLOCKING-ly or NON_BLOCKING-ly or with a timeout_in_microseconds
  * It allows you to downgrade writer lock to reader lock and upgrade reader lock to writer lock (with safety from deadlocks arising out of concurrent upgraders)
  * It allows you to have an external lock allowing you to build complex functionalities aroung this lock (see my projects Bufferpool and WALe)

2. A generalized lock-compatibility-matrix based lock short for glock
  * This lock works in cases when you have large number of locking-modes to access your data
  * It gets its rules from a lock-compatibility-matrix, that dictates which lock-modes are compatible and can exist concurrently
  * It can simulate large number of access patterns, and block conflicting or deadlocking operations, in highly granular lock based data-structures
  * For instance, think about a b+tree with per page/node level locks, in this situation you may have minimal access patterns like POINT_INSERT, POINT_DELETE, FORWARD_READ_SCAN, REVERSE_READ_SCAN, FORWARD_WRITE_SCAN, REVERSE_WRITE_SCAN, (scans here are leaf only scans).
    * if you look closely, the access patterns (lock_modes) do not fall into a strict read/write lock access pattern, because POINT_INSERT and POINT_DELETE can still be concurrently performed with FORWARD_READ_SCAN or BACKWARD_READ_SCAN, but similarly, a FORWARD_WRITE_SCAN can be concurrently performed with FORWARD_READ_SCAN but not with REVERSE_READ_SCAN or REVERSE_WRITE_SCAN (because of deadlocks ofcourse).
  * this is the problem glock solves, it defines what data-structure operations can happen concurrently and block the incompatible ones

**Now the following two question might pop up in your head**
  * *WHEN A rwlock CAN BE IMPLEMENTED USING THE glock, THEN WHY IS THERE A SEPARATE IMPLEMENTATION FOR A rwlock?*
  * *OR Why rwlock WILL NEVER BE IMPLEMENTED AS A GENERALIZED CASE OF glock?*
  * *The reasons are noted below:*
    * glock will not have separate condition variable for all locking modes, so can not prevent thread thrashing
    * glock will not have read/write preferring options
    * glock_transition_lock_mode may allow upgrade/downgrade, but does not protect against deadlock caused by 2 concurrent readers trying to upgrade the same reader lock, while rwlock gracefully fails such a case by only allowing exactly 1 thread to wait for upgrading the reader lock
    * glock needs to allocate and intialize a dynamic array for counts of locks issued per lock mode, and so it's initialization could fail, unlike rwlock
    * ***The star of this repository is still the rwlock***

## Setup instructions
**Install dependencies :**
  * [Cutlery](https://github.com/RohanVDvivedi/Cutlery)
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
   * `#include<lockking/rwlock.h>`
   * `#include<lockking/glock.h>`

## Instructions for uninstalling library

**Uninstall :**
 * `cd ReaderWriterLock`
 * `sudo make uninstall`
