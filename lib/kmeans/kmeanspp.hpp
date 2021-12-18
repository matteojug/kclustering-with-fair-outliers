#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "../kmeans.hpp"

namespace KMeans {

template <typename _InitialCenters>
struct KMeanspp : public BaseAlgorithm { // k-means++
    double eps_break = -1;
    KMeanspp(float eps_break = -1) : eps_break(eps_break) {}
    virtual string Codename() {
        return "kmeans++["s+_InitialCenters::name+"]("+to_string(seed)+","+to_string(run_count)+")";
    }
    virtual string Fullname() {
        return "KMeans::Kmeans++["s+_InitialCenters::name+"](seed="+to_string(seed)+",run_count="+to_string(run_count)+")";
    }

    virtual vector<Point> GetCentersPt(Instance* instance, Solution& solution, AlgorithmContext* context) {
        vector<Point> centers;
        for (const auto& p : _InitialCenters::centers(instance, context))
            centers.push_back(instance->data->points[p]);

        Distance cost = INF;
        int iter = 0;
        while (true) {
            iter++;
            unordered_map<int, vector<int>> clusters;
            for (auto& i : instance->data_points) {
                int center = instance->DistCenter(i, centers);
                clusters[center].push_back(i);
            }
            centers.clear();
            Distance new_cost = 0;
            int ic = 0;
            for (const auto& kv : clusters) {
                Point mean = instance->data->points[kv.second.front()];
                for (int i = 1; i < kv.second.size(); i++)
                    for (int j = 0; j < mean.x.size(); j++)
                        mean.x[j] += instance->data->points[kv.second[i]].x[j];
                for (int j = 0; j < mean.x.size(); j++)
                    mean.x[j] /= kv.second.size();
                
                centers.push_back(mean);
                for (const auto& vid : kv.second)
                    new_cost += pow(instance->Dist(vid, mean), 2);
            }
            // cerr<<"[iter:"<<iter<<"] Sol cost: "<<new_cost<<" (prev:"<<cost<<", improv:"<<cost/new_cost<<")"<<endl;
            if (new_cost >= cost) break;
            if (eps_break >= 0 && cost/new_cost < eps_break) break;
            cost = new_cost;
        }
        solution.extra["iter"] = to_string(iter);
        return centers;
    }
};

}