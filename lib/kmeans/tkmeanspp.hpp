#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "../kmeans.hpp"

namespace KMeans {

// Greedy Sampling for Approximate Clustering in the Presence of Outliers
struct Tkmeanspp : public BaseAlgorithm { // T-kmeans++
    _IGNORE_RUNCOUNT
    
    float beta;
    int iters;

    Tkmeanspp(int iters, float beta) : iters(iters), beta(beta) {}
    
    virtual string Codename() {
        return "tkmeans++("s+to_string(iters)+","+to_string(beta)+","+to_string(seed)+","+to_string(run_count)+")";
    }
    virtual string Fullname() {
        return "KMeans::T-Kmeans++(iters="s+to_string(iters)+",beta="+to_string(beta)+",seed="+to_string(seed)+",run_count="+to_string(run_count)+")";
    }

    virtual void GetCenters(Instance* instance, Solution& solution, AlgorithmContext* context) {
        auto opt_guesses = instance->OptGuesses(2);
        solution.extra["opt_guesses"] = to_string(opt_guesses.size());
        int cluster_iters = max(1,iters/(int)opt_guesses.size());

        int iter = 0;
        vector<int> best_centers = solution.centers;
        Distance best_cost = INF;
        for (auto& opt : opt_guesses) {
            auto cost = Cluster(instance, solution, context, opt, cluster_iters, &iter);
            if (cost < best_cost)
                // cout<<"New best:"<<best_cost<<endl,
                best_cost = cost, best_centers = solution.centers;
        }
        solution.centers = best_centers;
        solution.extra["iter"] = to_string(iter);
    }

    Distance Cluster(Instance* instance, Solution& solution, AlgorithmContext* context, Distance opt, int cluster_iters, int* iter_glob = nullptr) {
        Distance best_cost = INF;
        for (int iter = 0; iter < cluster_iters; iter++) {
            if (iter_glob != nullptr) (*iter_glob)++;
            set<int> centers;
            centers.insert(instance->data_points[std::uniform_int_distribution<>(0,instance->data_points.size()-1)(context->generator)]);
            for (int i = 1; i < instance->K; i++) {
                vector<Distance> dists;
                for (int i = 0; i < instance->data_points.size(); i++)
                    dists.push_back(min(static_cast<Distance>(pow(instance->Dist(instance->data_points[i], centers),2)),static_cast<Distance>(beta*opt/instance->Z)));
                centers.insert(instance->data_points[discrete_distribution<>(dists.begin(), dists.end())(context->generator)]);
            }
            auto cost = CostAuto(instance, centers);
            // cout<<"["<<iter<<"]: "<<cost<<" <?= "<<opt<<endl;
            if (cost < best_cost)
                best_cost = cost, solution.centers = {centers.begin(), centers.end()};
        }
        return best_cost;
    }
};

}