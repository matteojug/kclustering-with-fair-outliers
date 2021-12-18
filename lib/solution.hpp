#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "utils.hpp"
#include "defs.hpp"
#include "instance.hpp"

struct Solution {
    Instance *instance;
    string problem;
    string algo_codename, algo_fullname;

    // A solution is either a vec of points index (wrt instance->data)
    vector<int> centers; // as pt index, not id
    // Or a vec of arbitrary points; if this is non empty, centers is ignored
    vector<Point> centers_pt;

    vector<int> outliers, original_outliers; // as pt index, not id
    Distance cost, outliers_cost;

    vector<vector<int>> clusters; // as pt index, but is also dumped as index (not id). sorry.
    
    map<string,string> extra;

    float elapsed_ms = -1;

    virtual string Codename() {
        return instance->codename + "_" + algo_codename;
    }

    virtual void Info() {
        cout<<Codename()<<" @ "<<algo_codename<<"["<<algo_fullname<<"]"<<endl;
        cout<<"\tn="<<instance->data->points.size()<<",k="<<instance->K<<",z="<<instance->Z<<endl;
        cout<<"\tCost: "<<cost<<"(outlier cost:"<<outliers_cost<<"), centers:"<<centers.size()<<", centers_pt:"<<centers_pt.size()<<", outliers:"<<outliers.size()<<" (original outliers: "<<original_outliers.size()<<")"<<endl;
        cout<<"\tElapsedMs: "<<elapsed_ms<<endl;
    }
    vector<Point> GetCentersPt() {
        if (!centers_pt.empty()) return centers_pt;
        vector<Point> pts;
        for (auto& pt : centers)
            pts.push_back(instance->data->points[pt]);
        return pts;
    }
    virtual void ToJson(rapidjson::Value& root, rapidjson::Document::AllocatorType& allocator) const {
        root.AddMember("problem", rapidjson::Value(problem.c_str(), allocator), allocator);
        root.AddMember("algo_codename", rapidjson::Value(algo_codename.c_str(), allocator), allocator);
        root.AddMember("algo_fullname", rapidjson::Value(algo_fullname.c_str(), allocator), allocator);
        
        rapidjson::Value instance_obj(rapidjson::kObjectType);
        instance->ToJson(instance_obj, allocator);
        root.AddMember("instance", instance_obj, allocator);

        root.AddMember("elapsed_ms", elapsed_ms, allocator);
        root.AddMember("cost", cost, allocator);
        root.AddMember("outliers_cost", outliers_cost, allocator);

        rapidjson::Value centers_obj(rapidjson::kArrayType);
        for (const auto& x : centers)
            centers_obj.PushBack(instance->data->GetPointID(x), allocator);
        root.AddMember("centers", centers_obj, allocator);

        rapidjson::Value centers_pt_obj(rapidjson::kArrayType);
        for (const auto& pt : centers_pt) {
            rapidjson::Value pt_obj(rapidjson::kArrayType);
            for (const auto& x : pt.x)
                pt_obj.PushBack(x, allocator);
            centers_pt_obj.PushBack(pt_obj, allocator);
        }
        root.AddMember("centers_pt", centers_pt_obj, allocator);

        rapidjson::Value outliers_obj(rapidjson::kArrayType);
        for (const auto& x : outliers)
            outliers_obj.PushBack(instance->data->GetPointID(x), allocator);
        root.AddMember("outliers", outliers_obj, allocator);

        rapidjson::Value original_outliers_obj(rapidjson::kArrayType);
        for (const auto& x : original_outliers)
            original_outliers_obj.PushBack(instance->data->GetPointID(x), allocator);
        root.AddMember("original_outliers", original_outliers_obj, allocator);

        rapidjson::Value clusters_obj(rapidjson::kArrayType);
        for (const auto& cluster : clusters) {
            rapidjson::Value cluster_obj(rapidjson::kArrayType);
            for (const auto& v : cluster)
                cluster_obj.PushBack(v, allocator);
            clusters_obj.PushBack(cluster_obj, allocator);
        }
        root.AddMember("clusters", clusters_obj, allocator);

        rapidjson::Value extra_obj(rapidjson::kObjectType);
        for (const auto& x : extra)
            extra_obj.AddMember(rapidjson::Value(x.first.c_str(), allocator), rapidjson::Value(x.second.c_str(), allocator), allocator);
        root.AddMember("extra", extra_obj, allocator);
    }
    virtual string SavePath(const string& root) {
        return root + "/" + Codename() + ".json";
    }
    virtual void Save(const string& root) {
        string path = SavePath(root);

        const auto json = JsonToStrPtr(this);
        // std::cout << json.GetString() << std::endl;

        auto file = fopen(path.c_str(), "w");
        if (file == NULL){
            _fail("Unable to open file "<<path);
        }
        fwrite(json.GetString(), sizeof(char), json.GetSize(), file);
        fclose(file);
    }

};

struct SolutionShallow : public Solution {
    SolutionShallow(const string& path, const function<Dataset*(string)>& dataset_provider, bool dsname_is_fout=false) {
        FILE* fp = fopen(path.c_str(), "r");
        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        rapidjson::Document document;
        document.ParseStream(is);
        fclose(fp);
        FromJson(document, dataset_provider, dsname_is_fout);
    }
    // TODO: move to instance
    // TODO: colorfulness not loaded!
    Instance* InstanceFromJson(rapidjson::Value& root, const function<Dataset*(string)>& dataset_provider, bool dsname_is_fout) {
        Dataset* dataset;
        if (dsname_is_fout)
            dataset = dataset_provider(root["dataset"]["name_fout"].GetString());
        else
            dataset = dataset_provider(root["dataset"]["name"].GetString());
        
        Instance* instance;
        if (root["colorful"].GetBool() && !dsname_is_fout) {
            cout<<"Tried to load a colorful solution, why?"<<endl;
            assert(false);
        }
        if (root.HasMember("N_orig") && root["N"].GetInt() != root["N_orig"].GetInt()) {
            cout<<"Tried to load a restricted instance, why?"<<endl;
            assert(false);
        }
        if (dsname_is_fout && !root.HasMember("Z_perc")) {
            cout<<"This is not going to work, probably"<<endl;
            assert(false);
        }
        if (root.HasMember("Z_perc"))
            instance = new Instance(dataset, root["metric"].GetString(), root["K"].GetInt(), root["Z_perc"].GetFloat(), root["colorful"].GetBool());
        else
            instance = new Instance(dataset, root["metric"].GetString(), root["K"].GetInt(), root["Z"].GetInt());
        return instance;
    }
    void FromJson(rapidjson::Value& root, const function<Dataset*(string)>& dataset_provider, bool dsname_is_fout){
        // cout<<root["instance"]["dataset"]["name"].GetString()<<endl;
        // assert(false);
        problem = root["problem"].GetString();
        algo_codename = root["algo_codename"].GetString();
        algo_fullname = root["algo_fullname"].GetString();

        instance = InstanceFromJson(root["instance"], dataset_provider, dsname_is_fout);

        elapsed_ms = root["elapsed_ms"].GetFloat();
        cost = root["cost"].GetDouble();
        outliers_cost = root["outliers_cost"].GetDouble();

        centers.clear();
        for (auto& v : root["centers"].GetArray())
            centers.push_back(instance->data->GetPointIndex(v.GetInt64()));

        centers_pt.clear();
        for (auto& v : root["centers_pt"].GetArray()) {
            centers_pt.emplace_back();
            for (auto& x : v.GetArray())
                centers_pt.back().x.push_back(x.GetFloat());
        }
        
        outliers.clear();
        for (auto& v : root["outliers"].GetArray())
            outliers.push_back(instance->data->GetPointIndex(v.GetInt64()));

        original_outliers.clear();
        for (auto& v : root["original_outliers"].GetArray())
            original_outliers.push_back(instance->data->GetPointIndex(v.GetInt64()));

        clusters.clear();
        for (auto& v : root["clusters"].GetArray()) {
            clusters.emplace_back();
            for (auto& x : v.GetArray())
                clusters.back().push_back(x.GetInt64());
        }

        extra.clear();
        for (auto& kv : root["extra"].GetObject())
            extra[kv.name.GetString()] = kv.value.GetString();
    }

};