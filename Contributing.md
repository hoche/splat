# Contributing to SPLAT!

In general, the SPLAT! team welcomes contributions. If it's a small change, you can just file an Issue and post your snippet as an enhancement request. If it's a larger change, please send a patch. Either way, please indicate which branch you are coding against as we have several in flight at the moment.

## The Internal Test Suite

With Splat 2.0.0, gtest and gmock have been added and a number of unit tests have been implemented.

## The External Test Suite

A test suite is available at https://github.com/hoche/splat_testsuite. It uses python's unittest to compare a run of your current build of SPLAT! with archived results from the 1.4.2 code base. It is not by any means complete, but it is a useful tool for quickly flushing out any issues derived from code changes. 

It is bundled in a separate repository as both the datafiles is needs (SRTMs, etc) and the result files (various images, etc) can be rather large and there's no reason to force every user of SPLAT! to download them.

To use it, read its README. Essentially you can check it out to anywhere on your filesystem, set your local "SPLAT" environment variable to point to the binary you want to test against, and go.

## Coding Standards

In general, new code should follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), both for style and formatting, with the following notes:
* Use 4 spaces per indent, not 2. You should never use tabs. Configure your editor to emit spaces instead.
* Stick to C++11 or later paradigms.
* We are not currently using Boost. Stick the the C++ STL.
* Don't use #define guards in your headers. Just use "#pragma once". 
* Avoid "using namespace std". This brings in everything and we may have overridden some of the standard classes. If you don't want
  to keep retyping things like "std::cout" in code that has large blocks of emitted text, you may use "using std::cout" or similar
  in the block, but keep it local.
* Exceptions are allowed, with the following caveats:
  * Stick to instances of std::exception
  * Don't throw exceptions from destructors
  * Catch exceptions as soon as possible. Don't let them percolate all the way up the stack.
  Basically, follow the rules here: [https://www.perforce.com/resources/qac/high-integrity-cpp-coding-standard-exception-handling]

  Exceptions should be avoided in code that has to be highly-performant (like the ITWOM, etc).

## A note on Semantic Versioning (Semver)

As of SPLAT! 2.0.0, Semantic Versioning has been adopted. Given a version number MAJOR.MINOR.PATCH, the:

* MAJOR version will be updated when there are incompatible API changed (command line switches, etc)
* MINOR version will be updated when there is added functionality in a backward-compatible manner
* PATCH version will be updated when there is are backward-compatible bug fixes
    
Additional labels for pre-release and build metadata may be added as extensions to the MAJOR.MINOR.PATCH format.
