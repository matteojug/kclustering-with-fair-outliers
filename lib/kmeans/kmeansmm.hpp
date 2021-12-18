#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "../kmeans.hpp"

namespace KMeans {

template <typename _InitialCenters>
struct KMeansmm : public BaseAlgorithm { // k-means--
    virtual string Codename() {
        return "kmeans--["s+_InitialCenters::name+"]("+to_string(seed)+","+to_string(run_count)+")";
    }
    virtual string Fullname() {
        return "KMeans::Kmeans--["s+_InitialCenters::name+"](seed="+to_string(seed)+",run_count="+to_string(run_count)+")";
    }

    virtual vector<Point> GetCentersPt(Instance* instance, Solution& solution, AlgorithmContext* context) {
        vector<Point> centers;
        for (const auto& p : _InitialCenters::centers(instance, context))
            centers.push_back(instance->data->points[p]);
        
        int iter;
        centers = OutlierLloyd(instance, centers, -1, &iter);
        solution.extra["iter"] = to_string(iter);
        return centers;
    }
};

}