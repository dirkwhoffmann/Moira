// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "SSTTypes.h"
#include "SSTMoira.h"
#include "RefCore.h"
#include <map>
#include <set>
#include <string>

namespace sstrunner {

struct Counter {

    long cases = 0, ok = 0;
    std::map<std::string, long> signatures;
};

/* Base class of the SingleStepTests tools. A tool takes the instructions and the
 * initial states of the SingleStepTests, runs them on Moira and on a reference core
 * and compares the results. Which cases are derived from a test is decided by the
 * concrete runner:
 *
 *   PlainRunner      Runs the test as it is (the prerecorded results are ignored).
 *   ExceptionRunner  Lets exceptions hit in the middle of the instruction.
 */
class SSTRunner {

public:

    struct Settings {

        std::string dir;
        std::vector<std::string> only;
        long limit = 0;                 // Tests per file
        int verbose = 0;                // Failures to print per file
        int dedupe = 0;                 // Maximum number of cases per instruction form, access, and kind (0 = all)
        bool coverage = false;          // Report instruction forms without any case?
        double minPercent = 100.0;      // Required success rate (exit code)
        bool check = false;             // Only check the reference against the SingleStep expectations?
        std::string tsv;                // Optional result table
    };

    explicit SSTRunner(const CoreFactory &factory);
    virtual ~SSTRunner();

    // Parses the command line, runs all tests and returns the exit code of the tool
    int run(int argc, char **argv);


    //
    // Interface to be implemented by a concrete runner
    //

protected:

    // Text shown in the start-up banner
    virtual const char *description() const = 0;

    // Returns false if the runner cannot work with the given CPU model
    virtual bool acceptsModel(CPUModel model) const { return model == CPUModel::M68000; }

    // Returns false if a test is of no interest to the runner
    virtual bool accepts(const sst::Test &t) const { return true; }

    // Derives cases from a test and passes them to runCase()
    virtual void runTest(const sst::Test &t) = 0;


    //
    // Running a case
    //

    // Runs a case on the reference and on Moira and compares the results.
    // k: index of the failing bus access (-1 = none), kd: kind of the case, bus: bus error or address error?
    void runCase(const sst::Test &t, const sst::State &init, int k, int accesses, const mame::BusCycle &pk,
                 const char *kd, bool isFetch, bool bus, u16 plainSR, const std::string &label);

    // Compares the bus cycles of a test with the SingleStep expectations
    std::string compareBus(const sst::Test &t, const std::vector<mame::BusCycle> &got, bool dropAborted);

    // Runs the reference alone against the SingleStep expectations
    int checkReference(const std::vector<std::string> &names);

    Settings opt;
    CPUModel model = CPUModel::M68000;
    Reference ref;
    TestCPU moira;

    // Tolerances
    u32 srMask = 0xA71F;                // Compared bits of the status register
    u16 codeMask = 0xFFFF;              // Compared bits of the first word of bus and address error frames
    bool ignoreCycles = false;          // Skip the comparison of cycle counts and bus cycle timing?
    std::set<std::string> ignoredFields;    // Frame fields that are not compared (e.g., frame.addr)

    // Statistics
    Counter c;                          // Counter of the current file
    long failedReads = 0, failedWrites = 0;
    int shown = 0;
    std::map<std::string, Counter> byKind;
    std::map<std::string, long> signatures;
    std::map<std::string, int> keyCount;
    std::set<std::string> allForms, hitForms;
    long skipped = 0;                   // Cases that cannot be compared
    std::string skipNote;               // Why they cannot
    FILE *tsv = nullptr;
};


//
// Concrete runners
//

class PlainRunner : public SSTRunner {

public:

    using SSTRunner::SSTRunner;

protected:

    const char *description() const override;
    bool acceptsModel(CPUModel model) const override { return true; }
    void runTest(const sst::Test &t) override;
};

class ExceptionRunner : public SSTRunner {

public:

    explicit ExceptionRunner(const CoreFactory &factory);

protected:

    const char *description() const override;
    bool acceptsModel(CPUModel model) const override { return model == CPUModel::M68000 || model == CPUModel::M68020; }
    bool accepts(const sst::Test &t) const override;
    void runTest(const sst::Test &t) override;
};


// Helpers
bool expectsAddressError(const sst::Test &t);
const char *kind(const mame::BusCycle &c);

}
