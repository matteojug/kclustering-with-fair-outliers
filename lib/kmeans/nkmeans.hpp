#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "../kmeans.hpp"

namespace KMeans {

#define MAX_BALL_DIM 10

struct BallGrid {
    Instance *instance;
    Distance r;
    
    int dim, size = 0;
    unordered_map<vector<int>,vector<int>> grid;
    bool use_weights;

    BallGrid() {}
    BallGrid(Instance *instance, Distance r, int color = -1, bool use_weights_ = false) : instance(instance), r(r), use_weights(use_weights_) {
        dim = min(MAX_BALL_DIM,instance->data->dim);
        for (auto& i : instance->data_points) {
            if (color != -1 && instance->PointColor(i) != color) continue;
            size++;
            auto bucket = Bucket(i);
            grid[bucket].push_back(i);
            while (bucket.size()) {
                bucket.pop_back();
                grid[bucket];
            }
        }
        if (instance->data->point_weights.empty()) use_weights = false;
    }
    vector<int> Bucket(int pt) {
        vector<int> b;
        for (auto x : instance->data->points[pt].x) {
            b.push_back(ceil(x/r));
            if (b.size() == dim) break;
        }
        return b;
    }
    int query(int pt, int threshold) {
        int ball = 0;
        vector<int> bucket_orig = Bucket(pt), bucket;
        auto check = [&]() {
            for (auto& j : grid[bucket]) {
                if (instance->Dist(pt,j) <= r) {
                    if (!use_weights)
                        ball++;
                    else
                        ball += instance->data->point_weights[j];
                }
                if (ball >= threshold) break;
            }
        };
        function<void(int)> rec;
        rec = [&](int i) {
            if (i == dim) {
                check();
                return;
            }
            bucket.push_back(bucket_orig[i]);
            for (auto delta : {0,-1,+1}) {
                bucket[i] += delta;
                if (grid.count(bucket))
                    rec(i+1);
                bucket[i] -= delta;
                if (ball >= threshold) return;
            }
            bucket.pop_back();
        };
        rec(0);
        return ball;
    }
};

struct NKMeans : public BaseAlgorithm { // nk-means + kmeans++
    int iters;
    float guess_base = 2;

    NKMeans(int iters, float guess_base = 2) : iters(iters), guess_base(guess_base) {}

    virtual string Codename() {
        return "nkmeans("s+to_string(iters)+","+to_string(guess_base)+","+to_string(seed)+","+to_string(run_count)+")";
    }
    virtual string Fullname() {
        return "KMeans::NKMeans(iters="s+to_string(iters)+",guess_base="+to_string(guess_base)+",seed="+to_string(seed)+",run_count="+to_string(run_count)+")";
    }

    virtual bool Runnable(Instance *instance) {
        return instance->Z > 0;
    }

    virtual vector<Point> GetCentersPt(Instance* instance, Solution& solution, AlgorithmContext* context) {
        auto opt_guesses = instance->OptGuesses(2, guess_base);
        solution.extra["opt_guesses"] = to_string(opt_guesses.size());

        int iter = 0;
        vector<Point> best_centers = solution.centers_pt;
        set<int> best_outliers;
        Distance best_cost = INF;
        for (auto& opt : opt_guesses) {
            auto r = 2*pow(opt/instance->Z, 0.5);
            auto threshold = 2*instance->Z;
            vector<int> heavy;
            BallGrid grid(instance,r,-1,false);
            for (auto& i : instance->data_points)
                if (grid.query(i,threshold) >= threshold)
                    heavy.push_back(i);
            cerr<<"Heavy size:"<<heavy.size()<<endl;

            set<int> outliers;
            int inliers = 0;
            for (auto& i : instance->data_points) {
                bool has_heavy = false;
                for (auto& p : heavy)
                    if (instance->Dist(i, p) <= r) {
                        has_heavy = true;
                        break;
                    }
                if (!has_heavy)
                    outliers.insert(i);
                else
                    inliers++;
            }
            if (inliers == 0) continue;
            if (outliers.size() > (3*instance->K+2)*instance->Z) continue;
            auto cost = Cluster(instance, solution, context, outliers, iters, &iter);
            cerr<<"NKMeans::GetCentersPt "<<opt<<" (g:"<<opt_guesses.size()<<"), r="<<r<<",t="<<threshold<<" => "<<inliers<<" + "<<outliers.size()<<" out, cost="<<cost<<endl;
            if (cost < best_cost)
                best_cost = cost, best_centers = solution.centers_pt, best_outliers = outliers;
            break;
        }
        solution.extra["iter"] = to_string(iter);
        solution.original_outliers = {best_outliers.begin(), best_outliers.end()};
        return best_centers;
    }
    
    Distance Cluster(Instance* instance, Solution& solution, AlgorithmContext* context, const set<int>& outliers, int cluster_iters, int* iter_glob = nullptr) {
        Distance best_cost = INF;
        Instance restricted_instance = *instance;
        restricted_instance.data_points.clear();
        for (auto& i : instance->data_points)
            if (!outliers.count(i))
                restricted_instance.data_points.push_back(i);
        
        for (int iter = 0; iter < cluster_iters; iter++) {
            if (iter_glob != nullptr) (*iter_glob)++;
            auto centers = KMeanspp<InitialCenters::D2>().GetCentersPt(&restricted_instance, solution, context);
            auto cost = CostColorless(&restricted_instance, centers, 0);
            if (cost < best_cost)
                solution.centers_pt = centers, best_cost = cost;
        }
        return best_cost;
    }
};

struct ColorfulNKMeans : public BaseAlgorithm { // nk-means + kmeans++
    int iters;
    float kmod = 1, zmod = 2, guess_base = 2;
    bool use_weights;

    ColorfulNKMeans(int iters, float kmod = 1, float zmod = 2, float guess_base = 2, bool use_weights = false) : iters(iters), kmod(kmod), zmod(zmod), guess_base(guess_base), use_weights(use_weights) {}

    virtual string Codename() {
        return "colorfulnkmeans("s+to_string(iters)+","+to_string(kmod)+","+to_string(zmod)+","+to_string(guess_base)+","+to_string(seed)+","+to_string(run_count)+(use_weights ? ",use_weights"s : ""s)+")";
    }
    virtual string Fullname() {
        return "KMeans::ColorfulNKMeans(iters="s+to_string(iters)+",kmod="s+to_string(kmod)+",zmod="s+to_string(zmod)+",guess_base="s+to_string(guess_base)+",seed="+to_string(seed)+",run_count="+to_string(run_count)+",use_weights="+to_string(use_weights)+")";
    }

    virtual bool Runnable(Instance *instance) {
        return instance->Z > 0;
    }

    virtual vector<Point> GetCentersPt(Instance* instance, Solution& solution, AlgorithmContext* context) {
        bool use_weights = ColorfulNKMeans::use_weights;
        if (instance->data->point_weights.empty()) use_weights = false;

        auto colors = instance->PointsColor();
        unordered_map<int,vector<int>> points_color;
        unordered_map<int,int> points_weight;
        int sum_weights = 0;
        for (auto& i : instance->data_points) {
            points_color[colors[i]].push_back(i);
            if (use_weights) {
                points_weight[colors[i]] += instance->data->point_weights[i];
                sum_weights += instance->data->point_weights[i];
            }
        }
        
        auto all_colors = instance->AllColors();
        set<int> outliers;
        Timer timer;
        for (auto &c : all_colors) {
            auto opt_guesses = instance->OptGuesses(2,guess_base,c);
            solution.extra["opt_guesses:"s+to_string(c)] = to_string(opt_guesses.size());
            
            vector<int> outliers_cand;
            for (auto& opt : opt_guesses) {
                auto r = 2*pow(opt/instance->Za[c], 0.5);
                auto threshold = 2*instance->Za[c];
                if (use_weights) {
                    threshold = ceil(2*instance->Z_perc*points_weight[c]);
                }
                vector<int> heavy;
                BallGrid grid = BallGrid(instance,r,c,use_weights);
                for (auto& i : points_color[c])
                    if (grid.query(i,threshold) >= threshold)
                        heavy.push_back(i);

                outliers_cand.clear();
                int inliers = 0, outliers_weight = 0;
                for (auto& i : points_color[c]) {
                    bool has_heavy = false;
                    for (auto& p : heavy)
                        if (instance->Dist(i, p) <= r) {
                            has_heavy = true;
                            break;
                        }
                    if (!has_heavy) {
                        outliers_cand.push_back(i);
                        if (use_weights) outliers_weight += instance->data->point_weights[i];
                    }
                    else
                        inliers++;
                }
                if (inliers == 0) continue;
                if (!use_weights) {
                    if (outliers_cand.size() > (kmod*3*instance->K+zmod)*instance->Za[c]) continue;
                } else {
                    if (outliers_weight > (kmod*3*instance->K+zmod)*instance->Z_perc*points_weight[c]) continue;
                }
                break;
            }
            for (auto i : outliers_cand)
                outliers.insert(i);
        }
        auto t1 = timer.ElapsedMs(); timer.Reset();
        int iter = 0;
        auto cost = Cluster(instance, solution, context, outliers, iters, &iter);
        auto t2 = timer.ElapsedMs();
        cerr<<"ColorfulNKMeans::GetCentersPt: "<<outliers.size()<<" out, cost="<<cost<<endl;
        cerr<<"Ball time: "<<t1<<", cluster time: "<<t2<<endl;
        solution.extra["iter"] = to_string(iter);
        solution.original_outliers = {outliers.begin(), outliers.end()};
        return solution.centers_pt;
    }
    
    Distance Cluster(Instance* instance, Solution& solution, AlgorithmContext* context, const set<int>& outliers, int cluster_iters, int* iter_glob = nullptr) {
        Distance best_cost = INF;
        Instance restricted_instance = *instance;
        restricted_instance.data_points.clear();
        for (auto& i : instance->data_points)
            if (!outliers.count(i))
                restricted_instance.data_points.push_back(i);
        
        for (int iter = 0; iter < cluster_iters; iter++) {
            if (iter_glob != nullptr) (*iter_glob)++;
            auto centers = KMeanspp<InitialCenters::D2>().GetCentersPt(&restricted_instance, solution, context);
            auto cost = CostColorless(&restricted_instance, centers, 0);
            if (cost < best_cost)
                solution.centers_pt = centers, best_cost = cost;
        }
        return best_cost;
    }
};

}