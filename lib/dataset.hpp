#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "utils.hpp"
#include "defs.hpp"

struct Dataset {
    string name, name_fout;
    map<string,string> meta;
    int size, dim;
    vector<string> colors_header;
    vector<Point> points;

    string path;

    map<PointID,int> pt_index;
    vector<map<string,int>> colors_map; // color-idx (from 0)
    vector<map<string,int>> colors_count;

    int color_index = -1; // There was a reason for the dataset-level color, probably was a mistake

    vector<int> point_weights;
    
    void Info() {
        cerr<<"# "<<name<<" ("<<name_fout<<")"<<endl;
        for (const auto& x : meta)
            cerr<<"#@"<<x.first<<":"<<x.second<<endl;
        cerr<<"Size: "<<size<<", dim: "<<dim<<endl;
        cerr<<"Colors: "<<colors_header.size()<<endl;
        for (int i = 0; i < colors_header.size(); i++) {
            cerr<<"["<<i<<(color_index == i ? "*" : "")<<"] "<<colors_header[i]<<"("<<colors_count[i].size()<<"): ";
            for (const auto& kv : colors_count[i])
                cerr<<kv.first<<"["<<colors_map[i][kv.first]<<"]:"<<kv.second<<", ";
            cerr<<endl;
        }
    }
    Dataset(const vector<Point>& points_) : points(points_) {
        assert(!points.empty());
        size = points.size();
        dim = points.front().x.size();
        colors_header.resize(points.front().colors.size());
        colors_map.resize(points.front().colors.size());
        colors_count.resize(points.front().colors.size());

        for (int i = 0; i < points.size(); i++) {
            pt_index[points[i].id] = i;
            for (int j = 0; j < points[i].colors.size(); j++) {
                string color_str = to_string(points[i].colors[j]);
                colors_count[j][color_str]++;
                colors_map[j][color_str] = points[i].colors[j];
            }
        }
    }
    Dataset(const string& path, bool path_as_fout = false) : path(path) {
        ifstream fin(path);
        assert(fin.good());

        stringstream ss;
        string tmp;
        while (true) {
            getline(fin, tmp);
            if (tmp == "") continue;
            if (starts_with(tmp, "#@")) {
                auto tmp_split = string_split(tmp.substr(2), ":");
                meta[tmp_split[0]] = tmp_split[1];
            }
            if (tmp[0] == '#') continue;
            ss<<tmp;
            break;
        }
        ss>>name>>size>>dim;
        points.resize(size);

        int colors;
        fin>>colors;
        colors_header.resize(colors);
        for (auto& color : colors_header)
            fin>>color;
        
        colors_map.resize(colors);
        colors_count.resize(colors);
        for (auto& p : points) {
            fin>>p.id;
            p.x.resize(dim);
            for (auto& x : p.x)
                fin>>x;
            p.colors.resize(colors);
            for (int i = 0; i < colors; i++) {
                string color_str;
                fin>>color_str;
                if (!colors_map[i].count(color_str)) {
                    int new_color_id = colors_map[i].size();
                    colors_map[i][color_str] = new_color_id;
                }
                p.colors[i] = colors_map[i][color_str];
                colors_count[i][color_str]++;
            }
        }
        for (int i = 0; i < points.size(); i++)
            pt_index[points[i].id] = i;
        
        name_fout = name;
        if (path_as_fout) {
            int name_idx = path.find_last_of('/');
            if (name_idx == path.npos) name_idx = -1;
            name_fout = path.substr(name_idx+1);
            name_fout = name_fout.substr(0, name_fout.size()-3);
        }
    }
    Dataset& Prune(int new_size) {
        size = new_size;
        points.resize(size);
        return *this;
    }
    Dataset& UsingColor(int color_index_) {
        color_index = color_index_;
        if (color_index < 0 || color_index >= colors_header.size()) {
            cout<<"Color "<<color_index<<" not found"<<endl;
            throw "Color not found";
        }
        return *this;
    }
    Dataset& UsingColor(string color_index_) {
        for (int i = 0; i < colors_header.size(); i++)
            if (color_index_ == colors_header[i])
                return UsingColor(i);
        cout<<"Color "<<color_index_<<" not found"<<endl;
        throw "Color not found";
    }
    bool HasColor() {
        return color_index != -1;
    }
    Coloring ColorFn() {
        if (color_index == -1) {
            cout<<"No color specified"<<endl;
            throw "Color not found";
        }
        return [color=color_index](const Point& pt){
            return pt.colors[color];
        };
    }

    PointID GetPointID(int point_index) {
        return points[point_index].id;
    }
    int GetPointIndex(PointID point_id) {
        return pt_index[point_id];
    }

    map<tuple<string,int,int>,pair<Distance,Distance>> minmax_dists;
    void LoadMinMaxDists(bool verbose = false) {
        ifstream fin(path+".dists");
        assert(fin.good());
        string metric, color_h, color;
        Distance dmin, dmax;
        while (fin>>color_h) {
            if (color_h[0] == '#') {
                metric = color_h.substr(1);
                fin>>dmin>>dmax;
                minmax_dists[{metric, -1, -1}] = {dmin, dmax};
            } else {
                fin>>color>>dmin>>dmax;
                int cid_h = -1;
                for (int i = 0; i < colors_header.size(); i++)
                    if (color_h == colors_header[i])
                        cid_h = i;
                assert(cid_h != -1);
                assert(colors_map[cid_h].count(color));
                minmax_dists[{metric, cid_h, colors_map[cid_h][color]}] = {dmin, dmax};
            }
        }

        if (verbose)
            for (auto& kv : minmax_dists)
                cerr<<"("<<get<0>(kv.first)<<","<<get<1>(kv.first)<<","<<get<2>(kv.first)<<") "<<
                        kv.second.first<<","<<kv.second.second<<endl;
    }
    pair<Distance,Distance> MinMaxDists(const string& metric, Metric& dist_fn, int max_iters, int color = -1, int color_index_ = -2) {
        // info("minmax", metric<<","<<max_iters<<","<<color<<","<<color_index);
        if (color_index_ == -2) color_index_ = color_index;
        tuple<string,int,int> key = {metric, color_index, color};
        if (minmax_dists.count(key)) return minmax_dists[key];

        assert(max_iters != -1);
        pair<Distance,Distance> d = {INF, -INF};
        mt19937 gen(42);
        vector<int> pts;
        // info("dbg",size<<","<<color_index_);
        for (int i = 0; i < size; i++)
            if (color == -1 || points[i].colors[color_index_] == color)
                pts.push_back(i);

        auto rand_point = std::bind(std::uniform_int_distribution<>(0,pts.size()-1), std::ref(gen));
        for (int i = 0; i < min((int)pts.size(), max_iters); i++) {
            int u = pts[rand_point()];
            for (auto v : pts) {
                auto duv = dist_fn(points[u], points[v]);
                if (duv == 0) continue;
                d.first = min(d.first, duv);
                d.second = max(d.second, duv);
            }
        }
        // cout<<name<<"["<<metric<<","<<color<<"] minmax: "<<d.first<<","<<d.second<<endl;
        return minmax_dists[key] = d;
    }
    void ToJson(rapidjson::Value& root, rapidjson::Document::AllocatorType& allocator) const {
        root.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
        root.AddMember("name_fout", rapidjson::Value(name_fout.c_str(), allocator), allocator);
        rapidjson::Value meta_obj(rapidjson::kObjectType);
        for (const auto& x : meta)
            meta_obj.AddMember(rapidjson::Value(x.first.c_str(), allocator), rapidjson::Value(x.second.c_str(), allocator), allocator);
        root.AddMember("meta", meta_obj, allocator);

        root.AddMember("size", size, allocator);
        root.AddMember("dim", dim, allocator);

        root.AddMember("path", rapidjson::Value(path.c_str(), allocator), allocator);

        root.AddMember("color_index", color_index, allocator);
    }

};