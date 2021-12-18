#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "../kmeans.hpp"
#include "nkmeans.hpp"

namespace KMeans {

struct CoresetConfig {
    int seed = 42, iters = -1;
    int k_mult = 32;
};
Dataset* Coreset(Instance* instance, const CoresetConfig& cconfig) {
    assert(instance->colorful);
    
    default_random_engine rng(cconfig.seed);
    auto rand_unit = bind(uniform_real_distribution<double>(0.0, 1.0), std::ref(rng));
    AlgorithmContext context(cconfig.seed);

    auto colors = instance->PointsColor();
    unordered_map<int,vector<int>> points_color;
    for (auto& i : instance->data_points)
        points_color[colors[i]].push_back(i);
    
    int iters = cconfig.iters, 
        // exp_iters = 33*ceil(log2(points_color.size() * instance->data_points.size()));
        exp_iters = 33*ceil(log(points_color.size()));
    if (iters == -1)
        iters = exp_iters;
    info("coreset","Iters:"<<iters<<" (exp_iters:"<<exp_iters<<")");

    vector<Point> coreset;
    vector<int> coreset_weight;
    for (auto& kv : points_color) {
        vector<Point> coreset_a;
        int n_a = kv.second.size(), z_a = instance->Za[kv.first], k = instance->K;
        double p_a = max(36.0/z_a * log2(4*n_a*k*k/z_a), 36.0*k/z_a * log2(2*pow(k,3)));
        cerr<<"color "<<kv.first<<"("<<n_a<<"): p_a="<<p_a<<endl;

        Instance restricted_instance = *instance;
        Solution best;
        best.cost = INF;
        for (int h=0; h < iters; h++) {
            Solution sol_mono;
            restricted_instance.Z = 0;
            #ifdef FAST_CORESET
            p_a = min(2.5*k*max(1.0,log2(n_a))/z_a, 1.0);
            restricted_instance.data_points.clear();
            for (auto& i : kv.second)
                if (rand_unit() <= p_a)
                    restricted_instance.data_points.push_back(i);
            restricted_instance.K = ceil(k + p_a*z_a);
            #else
            if (p_a >= 1) {
                restricted_instance.data_points = kv.second;
                restricted_instance.K = cconfig.k_mult*(k + z_a);
            } else {
                restricted_instance.data_points.clear();
                for (auto& i : kv.second)
                    if (rand_unit() <= p_a)
                        restricted_instance.data_points.push_back(i);
                restricted_instance.K = ceil(cconfig.k_mult*(k + 2.5*p_a*z_a));
            }
            #endif
            cerr<<"monoinstance: n="<<restricted_instance.data_points.size()<<", k="<<restricted_instance.K<<endl;
            Timer timer;
            sol_mono.centers_pt = KMeanspp<InitialCenters::D2>(1.0001).GetCentersPt(&restricted_instance, sol_mono, &context);
            timer.PrintTime("computed kmeans++");
            cerr<<"monoinstance: sol size:"<<sol_mono.centers_pt.size()<<endl;
            restricted_instance.data_points = kv.second;
            restricted_instance.Z = 16*z_a;
            BaseAlgorithm().ClusterColorless(&restricted_instance, sol_mono);
            cerr<<"monoinstance: sol cost:"<<sol_mono.cost<<", best="<<best.cost<<endl;
            if (sol_mono.cost < best.cost)
                best = sol_mono;
        }
        best.instance = &restricted_instance;
        best.Info();
        for (int i = 0; i < best.centers_pt.size(); i++) {
            auto pt = best.centers_pt[i];
            pt.colors = {kv.first};
            pt.id = coreset.size();
            coreset.push_back(pt);
            coreset_weight.push_back(best.clusters[i].size());
        }
        // break;
    }
    info("coreset","Coreset size:"<<coreset.size()<<" (n="<<instance->data_points.size()<<")");
    auto ds = new Dataset(coreset);
    ds->UsingColor(0);
    ds->point_weights = coreset_weight;
    return ds;
}

}