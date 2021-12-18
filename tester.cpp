#include <bits/stdc++.h>
using namespace std;

#include "lib/kmeans.hpp"

#define ALGO_RUNS 1
 
struct Config {
    string ds, colorless, colorful, colored, fairz, coreset;
    int K, coreset_minmax_iters = 1000;
    double Z;
    bool verbose, overwrite;

    void Info() {
        cerr<<"ds:"<<ds<<endl<<
                "colorless:"<<colorless<<endl<<
                "colorful:"<<colorful<<endl<<
                "colored:"<<colored<<endl<<
                "fairz:"<<fairz<<endl<<
                "coreset:"<<coreset<<endl<<
                "K:"<<K<<endl<<
                "Z:"<<Z<<endl<<
                "coreset_minmax_iters:"<<coreset_minmax_iters<<endl<<
                "overwrite:"<<overwrite<<endl<<
                "verbose:"<<verbose<<endl;
    }
};
Config ParseConfig(int argc, char* argv[]){
    InitRawConfigMap(argc, argv, true);
    Config config;

    config.ds = GetConfig("ds","");
    if (config.ds == "") _fail("Missing ds arg")
    
    config.colorless = GetConfig("colorless","");
    if (config.colorless.size() && config.colorless.back() != '/') config.colorless += "/";

    config.colorful = GetConfig("colorful","");
    if (config.colorful.size() && config.colorful.back() != '/') config.colorful += "/";

    config.colored = GetConfig("colored","");
    if (config.colored.size() && config.colored.back() != '/') config.colored += "/";

    config.fairz = GetConfig("fairz","");
    if (config.fairz.size() && config.fairz.back() != '/') config.fairz += "/";

    config.coreset = GetConfig("coreset","");
    if (config.coreset.size() && config.coreset.back() != '/') config.coreset += "/";
    
    config.K = GetConfig("k",0);
    if (config.K == 0) _fail("Missing k arg");
    
    config.Z = GetConfig("z",-1.);
    if (config.Z < 0) _fail("Missing z arg");
    
    config.overwrite = GetConfig("overwrite",false);
    config.verbose = GetConfig("verbose",true);

    return config;
}

Dataset* dataset;
map<string, Dataset*> dataset_colored;
map<string, Dataset*> dataset_coresets;
map<string, long long> dataset_coresets_elapsed;

void DoColorless(Instance* instance, const Config& config) {
    vector<Algorithm*> algos = {
        (new KMeans::Random())->WithSeed(44)->WithRunCount(ALGO_RUNS),
        (new KMeans::KMeanspp<KMeans::InitialCenters::D2>())->WithSeed(44)->WithRunCount(ALGO_RUNS),
        (new KMeans::KMeansmm<KMeans::InitialCenters::D2>())->WithSeed(44)->WithRunCount(ALGO_RUNS),
        (new KMeans::Tkmeanspp(ALGO_RUNS,1))->WithSeed(44),
        (new KMeans::NKMeans(ALGO_RUNS))->WithSeed(44),
    };

    info("algos", "Algorithms:");
    for (auto& algo : algos)
        info("algos", algo->Codename()<<" => "<<algo->Fullname());
    
    for (auto& algo : algos) {
        if (!algo->Runnable(instance))
            continue;
        info("algos", algo->Codename()<<" => "<<algo->Fullname());
        
        auto sol_path = algo->SavePath(config.colorless, instance);
        if (!file_exists(sol_path) || config.overwrite) {
            cerr<<"# colorless:"<<endl;
            auto sol = algo->Run(instance);
            sol.Info();
            sol.Save(config.colorless);
        }
        
        Solution sol = SolutionShallow(sol_path, [&](string dsname){
            return dataset;
        });

        for (auto& kv : dataset_colored) {
            Instance instance_color(kv.second, sol.instance->metric, sol.instance->K, sol.instance->Z_perc, true);
            
            if (!config.colored.empty()) {
                cerr<<"# colored:"<<endl;
                Algorithm* algo = new KMeans::Fake(sol);
                auto new_sol = algo->Run(&instance_color);
                new_sol.extra["original_sol"] = sol_path;
                new_sol.elapsed_ms = sol.elapsed_ms;
                sol.Info();
                new_sol.Info();
                new_sol.Save(config.colored);
                delete algo;
            }
        
            if (!config.fairz.empty()) {
                cerr<<"# fairz:"<<endl;
                int real_z = instance_color.Z;
                auto instance_fairz = Instance(kv.second, sol.instance->metric, sol.instance->K, sol.instance->Z_perc, false);
                instance_fairz.Z = real_z;
                Algorithm* algo = new KMeans::Fake(sol);
                auto new_sol = algo->Run(&instance_fairz);
                new_sol.extra["original_sol"] = sol_path;
                new_sol.elapsed_ms = sol.elapsed_ms;
                new_sol.Info();
                new_sol.Save(config.fairz);
                delete algo;
            }
        }
    }
}

void DoColorful(Instance* instance, const Config& config) {
    vector<Algorithm*> algos = {
        (new KMeans::ColorfulNKMeans(ALGO_RUNS,0,1,1.1))->WithSeed(44),
    };

    info("algos", "Algorithms:");
    for (auto& algo : algos)
        info("algos", algo->Codename()<<" => "<<algo->Fullname());
    
    for (auto& algo : algos) {
        if (!algo->Runnable(instance))
            continue;
        
        auto sol_path = algo->SavePath(config.colorful, instance);
        if (!file_exists(sol_path) || config.overwrite) {
            cerr<<"# colorful:"<<endl;
            auto sol = algo->Run(instance);
            sol.Info();
            sol.Save(config.colorful);
        }
    }
}

void DoCoreset(Instance* instance, const Config& config) {
    vector<Algorithm*> algos = {
        // ==  Colorless baselines 
        (new KMeans::Random())->WithSeed(44)->WithRunCount(ALGO_RUNS),
        (new KMeans::KMeanspp<KMeans::InitialCenters::D2>())->WithSeed(44)->WithRunCount(ALGO_RUNS),
        (new KMeans::KMeansmm<KMeans::InitialCenters::D2>())->WithSeed(44)->WithRunCount(ALGO_RUNS),
        (new KMeans::Tkmeanspp(ALGO_RUNS,1))->WithSeed(44),
        (new KMeans::NKMeans(ALGO_RUNS))->WithSeed(44),

        // ==  Colorful
        (new KMeans::ColorfulNKMeans(ALGO_RUNS,0,1,1.1))->WithSeed(44),
    };

    info("algos", "Algorithms:");
    for (auto& algo : algos)
        info("algos", algo->Codename()<<" => "<<algo->Fullname());
    
    for (auto& algo : algos) {
        if (!algo->Runnable(instance))
            continue;
        
        for (auto& kv : dataset_colored) {
            if (!dataset_coresets.count(kv.first)) continue;

            Instance instance_color(kv.second, instance->metric, instance->K, instance->Z_perc, true);
            auto sol_path = algo->SavePath(config.coreset, &instance_color);
            if (file_exists(sol_path) && !config.overwrite) continue;

            cerr<<"# coreset:"<<endl;
            auto instance_coreset = Instance(dataset_coresets[kv.first], instance->metric, instance->K, instance->Z_perc, true);
            cerr<<"Coreset size:"<<instance_coreset.data_points.size()<<endl;
            auto sol_coreset = algo->Run(&instance_coreset);
            sol_coreset.Info();

            sol_coreset.centers_pt = sol_coreset.GetCentersPt();
            sol_coreset.centers.clear();
            sol_coreset.outliers.clear();

            Algorithm* algo = new KMeans::Fake(sol_coreset);
            auto new_sol = algo->Run(&instance_color);
            new_sol.extra["coreset_size"] = to_string(instance_coreset.data_points.size());
            new_sol.extra["elapsed_ms_coreset_construction"] = to_string(dataset_coresets_elapsed[kv.first]);
            new_sol.elapsed_ms = sol_coreset.elapsed_ms;
            
            new_sol.Info();
            new_sol.Save(config.coreset);
            delete algo;
        }
    }
}
int main(int argc, char* argv[]){
    auto config = ParseConfig(argc, argv);
    if (config.verbose) {
        info("parsed_config", "Config:");
        config.Info();
    }

    dataset = new Dataset(config.ds);
    if (config.verbose) {
        info("ds", "Dataset:");
        dataset->Info();
    }
    dataset->LoadMinMaxDists();

    for (auto color : dataset->colors_header) {
        cerr<<"Init color ds:"<<color<<endl;
        auto ds = new Dataset(config.ds);
        ds->UsingColor(color);
        ds->name_fout += "("s + color +")";
        ds->minmax_dists = dataset->minmax_dists;
        dataset_colored[color] = ds;
    }

    auto instance_colorless = Instance(dataset, Metrics::L2, config.K, config.Z, false);
    
    if (config.Z > 0 && !config.coreset.empty()) {
        KMeans::CoresetConfig cconfig;
        cconfig.iters = 1;
        for (auto color : dataset->colors_header) {
            auto instance_colored = Instance(dataset_colored[color], Metrics::L2, config.K, config.Z, true);
            Timer timer;
            auto coreset_ds = KMeans::Coreset(&instance_colored, cconfig);
            dataset_coresets_elapsed[color] = timer.ElapsedMs();
            coreset_ds->Info();
            auto d = coreset_ds->MinMaxDists(Metrics::L2, metrics[Metrics::L2], config.coreset_minmax_iters);
            for (auto& kv : coreset_ds->colors_map[coreset_ds->color_index]) {
                auto d = coreset_ds->MinMaxDists(Metrics::L2, metrics[Metrics::L2], config.coreset_minmax_iters, kv.second);
            }
            dataset_coresets[color] = coreset_ds;
        }
    }

    if (!config.colorless.empty())
        DoColorless(&instance_colorless, config);

    if (!config.colorful.empty()) {
        for (auto color : dataset->colors_header) {
            auto instance_colored = Instance(dataset_colored[color], Metrics::L2, config.K, config.Z, true);   
            DoColorful(&instance_colored, config);
        }
    }

    if (!config.coreset.empty())
        DoCoreset(&instance_colorless, config);

    return 0;
}
