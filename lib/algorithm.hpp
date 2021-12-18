#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "utils.hpp"
#include "defs.hpp"
#include "instance.hpp"
#include "solution.hpp"

struct AlgorithmContext {
    // Just smt to propagate the random state
    int run_id;
    
    mt19937 generator;

    AlgorithmContext(int seed) : generator(seed) {}
};

struct Algorithm {
    int run_count = 1;
    int seed = 42;
    
    // Return the algorithm codename (filename-friendly) and full name
    virtual string Codename() = 0;
    virtual string Problem() = 0;
    virtual string Fullname() = 0;

    // Compute solution. Solution is not polluted initially
    virtual void GetSolution(Instance* instance, Solution& solution, AlgorithmContext* context) = 0;

    virtual AlgorithmContext* InitContext() {
        return new AlgorithmContext(seed);
    }
    virtual Algorithm* WithRunCount(int run_count_) {
        run_count = run_count_;
        return this;
    }
    virtual Algorithm* WithSeed(int seed_) {
        seed = seed_;
        return this;
    }
    virtual bool Runnable(Instance *instance) {
        return true;
    }
    Solution Run(Instance *instance) {
        Solution sol;
        sol.cost = INF;
        Timer timer;
        auto ctx = InitContext();
        for (ctx->run_id = 0; ctx->run_id < run_count; ctx->run_id++) {
            Solution sol_tmp;
            GetSolution(instance, sol_tmp, ctx);
            if (sol.cost < sol_tmp.cost)
                continue;
            sol = sol_tmp;
        }
        delete ctx;
        sol.instance = instance;
        sol.problem = Problem();
        sol.algo_codename = Codename();
        sol.algo_fullname = Fullname();
        sol.elapsed_ms = timer.ElapsedMs();
        return sol;
    }

    string SavePath(const string& root, Instance *instance) {
        Solution sol;
        sol.instance = instance;
        sol.algo_codename = Codename();
        return sol.SavePath(root);
    }
    
    virtual ~Algorithm() = default;
};

#define _IGNORE_RUNCOUNT \
    virtual Algorithm* WithRunCount(int run_count_) { \
        run_count = 1; return this;  \
    }
