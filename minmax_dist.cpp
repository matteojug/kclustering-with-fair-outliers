#include <bits/stdc++.h>
using namespace std;

#include "lib/kmeans.hpp"

int main(int argc, char* argv[]){
    string ds_path = argv[1];
    int minmax_iters = argc > 2 ? atoi(argv[2]) : 10000;
    
    auto print_dists = [](const pair<Distance,Distance>& d) {
        cout<<std::setprecision(16)<<d.first<<"\t"<<d.second<<endl;
    };

    auto dataset = new Dataset(ds_path);
    dataset->Info();
    for (string metric_name : {Metrics::L1, Metrics::L2}) {
        cout<<"#"<<metric_name<<" ";
        print_dists(dataset->MinMaxDists(metric_name, metrics[metric_name], minmax_iters));
        for (int i = 0; i < dataset->colors_header.size(); i++) {
            dataset->UsingColor(i);
            for (auto& kv : dataset->colors_map[i]) {
                cout<<dataset->colors_header[i]<<" "<<
                        kv.first<<" ";
                print_dists(dataset->MinMaxDists(metric_name, metrics[metric_name], minmax_iters, kv.second));
            }
        }
    }

    return 0;
}