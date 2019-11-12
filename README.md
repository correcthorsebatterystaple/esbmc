[![Build Status](https://ssvlab.ddns.net/buildStatus/icon?job=esbmc-private2%2Fmaster)](https://ssvlab.ddns.net/buildStatus/icon?job=esbmc-private2%2Fmaster)

# The ESBMC model checker

ESBMC, the efficient SMT based model checker, is a software verification tool for C and C++ code bases. The technique is sound but incomplete -- an error found by ESBMC will be correct (modulo errors in the tool), but a successful verification does not guarantee there are no errors.

To build ESBMC, please see the BUILDING file. For getting started, we recommend first reading some of the background material / publications, to understand exactly what this technique can provide, for example our SV-COMP tool papers.

The cannonical public location of ESBMCs source is on github:

    https://github.com/esbmc/esbmc

While our main website is esbmc.org

### Features

ESBMC aims to support all of C99, and detects errors in software by simulating a finite prefix of the program execution with all possible inputs. Classes of problems that can be detected include:
 * User specified assertion failures
 * Out of bounds array access
 * Illegal pointer dereferences, such as:
   * Dereferencing null
   * Performing an out-of bounds dereference
   * Double-free of malloc'd memory
   * Misaligned memory access
 * Integer overflows
 * Divide by zero
 * Memory leaks

Concurrent software (using the pthread api) is verified by explicit exploration of interleavings, producing one symbolic execution per interleaving. By default only normal errors will be checked for; one can also specify options to check concurrent programs for:
 * Deadlock (only on pthread mutexes and convars)
 * Data races (i.e. competing writes)

By default ESBMC performs a "lazy" depth first search of interleavings -- it can also encode (explicitly) all interleavings into a single SMT formula.

A number of SMT solvers are currently supported:
 * Z3 4.0+
 * Boolector 2.0+
 * MathSAT
 * CVC4
 * Yices 2.2+

In addition, ESBMC can be configured to use the SMTLIB interactive text format with a pipe, to communicate with an arbitary solver process, although not-insignificant overheads are involved.

A limited subset of C++98 is supported too -- a library modelling the STL is also available.

### Differences from CBMC

ESBMC is based on CBMC, the C bounded model checker. The primary differences between the two are that CBMC focuses on SAT-based encodings of unrolled C programs while ESBMC targets SMT; and CBMC's concurrency support is a fully symbolic encoding of a concurrent program in one SAT formulae.

The fundemental verification technique (unrolling programs to SSA then converting to a formula) is still the same in ESBMC, although the program internal representation has been had some additional types added.

# Open source

ESBMC has now been released as open source software -- mainly distributed under the terms of the Apache License 2.0. ESBMC contains a signficant amount of other peoples software, however, please see the COPYING file for an explanation of who-owns-what, and under what terms they are distributed. Note that our K-induction implementation does not feature in this source release.

We fully intend on continuing to maintain and develop ESBMC. While it's open source, it is not necessarily open development, in that decisions on future direction will only be made by the developers, and the development tree is not part of the public distribution. Source releases are made by exporting a source distribution from the dev tree, which is then committed to the public git repository.

We'd be extremely happy to receive contributions to make ESBMC better (under the terms of the Apache License 2.0). If you'd like to submit anything, please file a pull request against the public github repo. General discussion and release announcements will be made on the esbmc-users@googlegroups.com mailing list. To contact us about research or collaboration, please email the esbmc@ mailing list on ecs.soton.ac.uk.

### Getting started

Currently, we don't have a good guide for getting started with ESBMC, although we hope to improve this in the future. Examining some of the benchmarks in the SV-COMP competition (http://sv-comp.sosy-lab.org/) would be a good start, using the esbmc command line for the relevant competition year.

### Documentation

A limited number of classes have been marked up with doxygen documentation headers. Comments are put in the header files declaring classes and methods. HTML documation can be generated by running:

    doxygen .doxygen

Output will be in docs/html, open index.html to get started. 
