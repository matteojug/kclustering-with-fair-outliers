#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "utils.hpp"
#include "defs.hpp"
#include "dataset.hpp"

struct Instance {
    Dataset *data;
    vector<int> data_points; // Default to {0,1,...,data->size-1}, may be changed to work on subsets of pts
    int K, Z;
    string metric;

    string codename;
    Metric metric_fn;

    float Z_perc = -1;
    vector<int> Za, Za_inverse; // inverse is the how-many-must-cover view
    
    bool colorful = false;
    Coloring color_fn;

    Instance(Dataset *data, const string& metric_, int K, int Z) : data(data), K(K), Z(Z), metric(metric_) {
        codename = data->name_fout+
                    "_k"+to_string(K)+
                    "_z"+to_string(Z)+
                    "_m"+metric;
        metric_fn = metrics[metric];
        InitDP();
    }
    // If data_points is changed, Z must be recomputed to keep the relation to Z_perc
    Instance(Dataset *data, const string& metric_, int K, float Z_perc, bool colorful_ = false) : data(data), K(K), Z(ceil(Z_perc*data->size)), Z_perc(Z_perc), metric(metric_), colorful(colorful_) {
        codename = data->name_fout+
                    "_k"+to_string(K)+
                    "_z"+fmt_float(Z_perc,4)+
                    "_m"+metric;
        metric_fn = metrics[metric];
        InitDP();

        if (colorful) {
            assert(data->HasColor());
            color_fn = data->ColorFn();
            Za.resize(data->colors_count[data->color_index].size());
            // info("", Za.size());
            Za_inverse.resize(Za.size());
            Z = 0;
            for (const auto& kv : data->colors_count[data->color_index]) {
                auto color_idx = data->colors_map[data->color_index][kv.first];
                // info("", kv.first<<":"<<color_idx);
                Za[color_idx] = ceil(Z_perc*kv.second);
                Z += Za[color_idx];
                Za_inverse[color_idx] = kv.second-Za[color_idx];
            }
        }
    }
    void InitDP() {
        data_points.clear();
        for (int i = 0; i < data->size; i++)
            data_points.push_back(i);
    }
    // If an algo needs to handle colors, use this
    vector<int> PointsColor() {
        vector<int> r;
        for (const auto& pt : data->points) // not on data_points, return all
            r.push_back(color_fn(pt));
        return r;
    }
    int PointColor(int pt_index) {
        return color_fn(data->points[pt_index]);
    }
    set<int> AllColors() {
        set<int> r;
        for (const auto& pt : data_points)
            r.insert(PointColor(pt));
        return r;
    }

    pair<Distance,Distance> MinMaxDists(int max_iters = 10000, int color = -1) {
        return data->MinMaxDists(metric, metric_fn, max_iters, color);
    }
    vector<Distance> OptGuesses(float p = 1, float base = 2, int color = -1) {
        auto minmax = MinMaxDists(-1, color);
        vector<Distance> v;
        for (int i = log(1e-9+data_points.size()*pow(minmax.first,p))/log(base); pow(base,i) <= data_points.size()*pow(minmax.second,p); i++)
            v.push_back(pow(base,i));
        return v;
    }

    // Dist (and related) work on point indices. The algo should not care about the original point ID
    Distance Dist(int a, int b) {
        return metric_fn(data->points[a], data->points[b]);
    }
    Distance Dist(int a, const Point& b) {
        return metric_fn(data->points[a], b);
    }
    Distance Dist(const Point& a, const Point& b) {
        return metric_fn(a, b);
    }
    template <typename T, typename U>
    Distance Dist(const T& a, const U& bs) {
        Distance d = INF;
        for (const auto& b : bs)
            d = min(d, Dist(a,b));
        return d;
    }
    template <typename T> // T: vector<x:Dist(int, x) is defined>
    int DistCenter(int a, const vector<T>& bs) { // return the index of the argmin
        Distance dist = INF;
        int best = -1;
        for (int i = 0; i < bs.size(); i++) {
            auto& b = bs[i];
            auto d = Dist(a,b);
            if (d < dist) {
                dist = d;
                best = i;
            }
        }
        return best;
    }
    
    void ToJson(rapidjson::Value& root, rapidjson::Document::AllocatorType& allocator) const {
        rapidjson::Value data_obj(rapidjson::kObjectType);
        data->ToJson(data_obj, allocator);
        root.AddMember("dataset", data_obj, allocator);

        root.AddMember("N", data_points.size(), allocator);
        root.AddMember("N_orig", data->points.size(), allocator);
        root.AddMember("K", K, allocator);
        root.AddMember("Z", Z, allocator);

        root.AddMember("colorful", colorful, allocator);
        if (Z_perc >= 0)
            root.AddMember("Z_perc", Z_perc, allocator);
        
        if (!Za.empty()) {
            rapidjson::Value za_obj(rapidjson::kArrayType);
            for (const auto& x : Za)
                za_obj.PushBack(x, allocator);
            root.AddMember("Z_a", za_obj, allocator);
        }

        root.AddMember("metric", rapidjson::Value(metric.c_str(), allocator), allocator);
        root.AddMember("codename", rapidjson::Value(codename.c_str(), allocator), allocator);
    }
};