#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "utils.hpp"
#include "defs.hpp"
#include "instance.hpp"
#include "solution.hpp"
#include "algorithm.hpp"

namespace KMeans {

namespace InitialCenters {
    struct UAR {
        inline static const string name = "UAR";
        static vector<int> centers(Instance* instance, AlgorithmContext* context) {
            auto rand_point = std::bind(std::uniform_int_distribution<>(0,instance->data_points.size()-1), std::ref(context->generator));
            set<int> centers;
            while (centers.size() < min(instance->K, (int)instance->data_points.size()))
                centers.insert(instance->data_points[rand_point()]);
            return vector<int>{centers.begin(), centers.end()};
        }
    };
    struct D2 {
        inline static const string name = "D2"; 
        static vector<int> centers(Instance* instance, AlgorithmContext* context) {
            vector<int> centers;
            centers.push_back(instance->data_points[std::uniform_int_distribution<>(0,instance->data_points.size()-1)(context->generator)]);
            vector<Distance> dists(instance->data_points.size(), INF);
            while (centers.size() < min(instance->K, (int)instance->data_points.size())) {
                int nonzero = 0;
                for (int i = 0; i < instance->data_points.size(); i++) {
                    Distance new_dist = pow(instance->Dist(instance->data_points[i], centers.back()),2);
                    if (new_dist < dists[i]) dists[i] = new_dist;
                    if (dists[i] > 0) nonzero++;
                }
                if (!nonzero) break;
                centers.push_back(instance->data_points[discrete_distribution<>(dists.begin(), dists.end())(context->generator)]);
            }
            return centers;
        }
    };
};

struct BaseAlgorithm : public Algorithm {
    virtual string Problem() { return "kmeans"; }
    virtual string Codename() { return "BaseAlgorithm"; }
    virtual string Fullname() { return "BaseAlgorithm"; }

    virtual vector<Point> GetCentersPt(Instance* instance, Solution& solution, AlgorithmContext* context) {
        assert(false); // shouldn't get there
    }
    // Calls GetCentersPt and map them back to the closest point in the dataset
    virtual void GetCenters(Instance* instance, Solution& solution, AlgorithmContext* context) {
        auto centers = GetCentersPt(instance, solution, context);
        solution.centers_pt.clear();
        for (const auto& c : centers) {
            solution.centers_pt.push_back(c);
            int closer = -1;
            Distance d = INF;
            for (auto& i : instance->data_points) {
                auto d2 = instance->Dist(i, c);
                if (d2 < d)
                    closer = i, d = d2;
            }
            solution.centers.push_back(closer);
        }
    }

    virtual void GetSolution(Instance* instance, Solution& solution, AlgorithmContext* context) {
        GetCenters(instance, solution, context);
        if (instance->colorful)
            ClusterColorful(instance, solution);
        else
            ClusterColorless(instance, solution);
    }

    void ClusterColorless(Instance* instance, Solution& solution) {
        priority_queue<pair<Distance,pair<int,int>>> outliers; // <distance,<point,center>>
        solution.cost = 0;
        solution.outliers_cost = 0;
        solution.clusters.clear();
        if (!solution.centers_pt.empty())
            solution.clusters.resize(solution.centers_pt.size());
        else
            solution.clusters.resize(solution.centers.size());

        for (auto& i : instance->data_points) {
            int center = -1;
            Distance d;
            if (!solution.centers_pt.empty()) {
                center =  instance->DistCenter(i, solution.centers_pt);
                d = instance->Dist(i, solution.centers_pt[center]);
            } else {
                center =  instance->DistCenter(i, solution.centers);
                d = instance->Dist(i, solution.centers[center]);
            }
            outliers.push({-d,{i,center}}); // pq is a max heap
            while (outliers.size() > instance->Z) {
                auto p = outliers.top();
                solution.cost += pow(-p.first, 2);
                solution.clusters[p.second.second].push_back(p.second.first);
                outliers.pop();
            }
        }
        while (!outliers.empty()) {
            auto p = outliers.top();
            solution.outliers.push_back(p.second.first);
            solution.outliers_cost += pow(-p.first, 2);
            outliers.pop();
        }
        // cout<<solution.cost<<" vs "<<Cost(instance, solution.centers, instance->Z)<<endl;
    }
    void ClusterColorful(Instance* instance, Solution& solution) {
        priority_queue<pair<Distance,pair<int,int>>> outliers; // <distance,<point,center>>
        solution.cost = 0;
        solution.outliers_cost = 0;
        solution.clusters.clear();
        if (!solution.centers_pt.empty())
            solution.clusters.resize(solution.centers_pt.size());
        else
            solution.clusters.resize(solution.centers.size());
        for (auto& i : instance->data_points) {
            int center = -1;
            Distance d;
            if (!solution.centers_pt.empty()) {
                center =  instance->DistCenter(i, solution.centers_pt);
                d = instance->Dist(i, solution.centers_pt[center]);
            } else {
                center =  instance->DistCenter(i, solution.centers);
                d = instance->Dist(i, solution.centers[center]);
            }
            outliers.push({d,{i,center}}); // pq is a max heap
        }
        auto budget = instance->Za;
        auto points_color = instance->PointsColor();
        while (!outliers.empty()) {
            auto dist = pow(outliers.top().first, 2);
            auto pt_idx = outliers.top().second.first;
            auto center = outliers.top().second.second; 
            int color = points_color[pt_idx];
            if (budget[color] == 0) {
                solution.cost += dist;
                solution.clusters[center].push_back(pt_idx);
            } else {
                budget[color]--;
                solution.outliers.push_back(pt_idx);
                solution.outliers_cost += dist;
            }
            outliers.pop();
        }
    }


    template <typename C, typename U>
    Distance CostColorless(Instance* instance, const C& centers, const U& pts, int Z = 0) {
        vector<Distance> costs;
        for (const auto& pt : pts)
            costs.push_back(pow(instance->Dist(pt, centers), 2));
        sort(costs.begin(), costs.end());
        Distance cost = 0;
        for (int i = 0; i < costs.size() - Z; i++)
            cost += costs[i];
        return cost;
    }
    template <typename C>
    Distance CostColorless(Instance* instance, const C& centers, int Z = 0) {
        vector<Distance> costs;
        for (auto& i : instance->data_points)
            costs.push_back(pow(instance->Dist(i, centers), 2));
        sort(costs.begin(), costs.end());
        Distance cost = 0;
        for (int i = 0; i < costs.size() - Z; i++)
            cost += costs[i];
        return cost;
    }

    template <typename C>
    Distance CostColorful(Instance* instance, const C& centers) {
        vector<pair<Distance,int>> costs;
        for (auto& i : instance->data_points)
            costs.push_back({pow(instance->Dist(i, centers), 2), i});
        sort(costs.begin(), costs.end());
        Distance cost = 0;
        auto budget = instance->Za;
        for (int i = costs.size()-1; i >= 0; i--) {
            int color = instance->PointColor(costs[i].second);
            if (budget[color] == 0) {
                cost += costs[i].first;
            } else {
                budget[color]--;
            }
        }
        return cost;
    }

    template <typename C>
    Distance CostAuto(Instance* instance, const C& centers) {
        if (instance->colorful)
            return CostColorful(instance, centers);
        else
            return CostColorless(instance, centers, instance->Z);
    }

};

struct Fake : public BaseAlgorithm {
    string codename, fullname;
    vector<Point> centers_pt;
    vector<PointID> centers, original_outliers; // Since point index may be different going from the original instance to the new instance (the one this is applied to), the points are saved by their ID (and mapped back to index in GetCenters)

    Fake(const Solution& sol) : codename(sol.algo_codename), fullname(sol.algo_fullname), centers_pt(sol.centers_pt) {
        for (auto c : sol.centers)
            centers.push_back(sol.instance->data->GetPointID(c));
        for (auto c : sol.outliers)
            original_outliers.push_back(sol.instance->data->GetPointID(c));
    }
    virtual string Codename() { return codename; }
    virtual string Fullname() { return fullname; }
    virtual vector<Point> GetCentersPt(Instance* instance, Solution& solution, AlgorithmContext* context) {
        cout<<"Fake::GetCentersPt, why?"<<endl;
        assert(false); // shouldn't get there
    }
    virtual void GetCenters(Instance* instance, Solution& solution, AlgorithmContext* context) {
        solution.centers_pt = centers_pt;
        for (auto c : centers)
            solution.centers.push_back(instance->data->GetPointIndex(c));
        for (auto c : original_outliers)
            solution.original_outliers.push_back(instance->data->GetPointIndex(c));
    }
};

struct Random : public BaseAlgorithm {
    virtual string Codename() {
        return "Rnd("s+to_string(seed)+","+to_string(run_count)+")";
    }
    virtual string Fullname() {
        return "KMeans::Random(seed="s+to_string(seed)+",run_count="+to_string(run_count)+")";
    }

    virtual void GetCenters(Instance* instance, Solution& solution, AlgorithmContext* context) {
        solution.centers = InitialCenters::UAR::centers(instance, context);
    }
};

vector<Point> OutlierLloyd(Instance* instance, vector<Point> centers, int max_iters = -1, int* iteration_count = nullptr) {
    Distance cost = INF;
    if (iteration_count != nullptr) *iteration_count = 0;
    while (max_iters--) {
        if (iteration_count != nullptr) (*iteration_count)++;
        vector<pair<int,pair<int,Distance>>> ordered_points;
        for (auto& i : instance->data_points) {
            int center = instance->DistCenter(i,centers);
            ordered_points.push_back({i,{center,instance->Dist(i,centers[center])}});
        }
        sort(ordered_points.begin(), ordered_points.end(), [](const pair<int,pair<int,Distance>>& a, const pair<int,pair<int,Distance>>& b) { return a.second.second < b.second.second; });

        unordered_map<int, vector<int>> clusters;
        for (int i = 0; i < instance->data_points.size()-instance->Z; i++)
            clusters[ordered_points[i].second.first].push_back(ordered_points[i].first);

        centers.clear();
        Distance new_cost = 0;
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
        // cerr<<"Sol cost: "<<new_cost<<" (prev:"<<cost<<")"<<endl;
        if (new_cost >= cost) break;
        cost = new_cost;
    }
    return centers;
}

}

#include "kmeans/kmeanspp.hpp"
#include "kmeans/kmeansmm.hpp"
#include "kmeans/tkmeanspp.hpp"
#include "kmeans/nkmeans.hpp"
#include "kmeans/coreset.hpp"