#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "utils.hpp"

#define INF 1e30

typedef double PointCoord;
typedef double Distance;
typedef int PointID;

struct Point {
    PointID id;
    vector<PointCoord> x;
    vector<int> colors;
};

typedef function<int(const Point&)> Coloring;

typedef function<Distance(const Point&,const Point&)> Metric;
namespace Metrics {
    Distance LP_fn(const Point& a, const Point&b, int p) {
        Distance d = 0;
        for (int i = 0; i < a.x.size(); i++)
            d += pow(abs(a.x[i]-b.x[i]), p);
        return pow(d, 1./p);
    }

    Metric L1_fn = bind(LP_fn, placeholders::_1, placeholders::_2, 1);
    Metric L2_fn = bind(LP_fn, placeholders::_1, placeholders::_2, 2);

    string L1 = "L1";
    string L2 = "L2";
}
map<string,Metric> metrics = {
    {Metrics::L1,Metrics::L1_fn},
    {Metrics::L2,Metrics::L2_fn},
};

