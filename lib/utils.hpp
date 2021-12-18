#pragma once

#include <bits/stdc++.h>
using namespace std;

#define info(tag, msg) cerr<<"["<<tag<<"]"<<__FILE__<<":"<<__LINE__<<"@"<<__func__<<": "<<msg<<endl
// #define info(tag, msg) // Uncomment to silent it
#define _fail(e) { cerr<<"[fail] " __FILE__":"<<__LINE__<<" @ "<<e<<endl; exit(1); }

#include "thirdparty/rapidjson/include/rapidjson/document.h"
#include "thirdparty/rapidjson/include/rapidjson/writer.h"
#include "thirdparty/rapidjson/include/rapidjson/stringbuffer.h"
#include "thirdparty/rapidjson/include/rapidjson/filereadstream.h"

#define JsonToStr(x) JsonToStr_([&x](rapidjson::Value& root, rapidjson::Document::AllocatorType& allocator){ return x.ToJson(root, allocator);})
#define JsonToStrPtr(x) JsonToStr_([x](rapidjson::Value& root, rapidjson::Document::AllocatorType& allocator){ return x->ToJson(root, allocator);})

rapidjson::StringBuffer JsonToStr_(function<void(rapidjson::Value&, rapidjson::Document::AllocatorType&)> json_fn){
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    json_fn(document, allocator);

    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    return strbuf;
}

string fmt_float(float x, int prec = 4) {
    stringstream stream;
    stream << fixed << setprecision(prec) << x;
    return stream.str();
}

inline bool file_exists(const string& name) {
    ifstream f(name);
    return f.good();
}

// Timer class to get times for algo phases
struct Timer {
    chrono::time_point<chrono::steady_clock> ts;
    Timer(){
        Reset();
    }
    void Reset(){
        ts = chrono::steady_clock::now();
    }
    long long ElapsedMs(){
        return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-ts).count();
    }
    void PrintTime(string tag){
        cerr<<"[T] "<<tag<<": "<<fmt_float(ElapsedMs()/1000., 3)<<"s"<<endl;
    }
};


//using boost::hash_combine
template <class T>
inline void hash_combine(std::size_t& seed, T const& v) {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
namespace std {
    template<typename T>
    struct hash<vector<T>> {
        typedef vector<T> argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& in) const {
            size_t seed = 0;
            for (const auto& x : in)
                hash_combine(seed, x);
            return seed;
        }
    };
}

bool starts_with(std::string mainStr, std::string toMatch) {
	return mainStr.find(toMatch) == 0;
}

vector<string> string_split(const string& input, const string& regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    return {first, last};
}

vector<string> tokenize(string const &in, char sep=' ') {
    string::size_type b = 0;
    vector<string> result;
    while ((b = in.find_first_not_of(sep, b)) != string::npos) {
        auto e = in.find_first_of(sep, b);
        result.push_back(in.substr(b, e-b));
        b = e;
    }
    return result;
}

typedef map<string,string> RawConfig;
RawConfig ConfigGlobal; // Yep, global
void InitRawConfigMap(int argc, char* argv[], bool verbose = true) {
    if (verbose) info("conf","Config:");
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] != '-') continue;
        char* s = argv[i]; s++;
        auto toks = tokenize(s, '=');
        if (verbose) info("conf","\t"<<toks[0]<<"="<<toks[1]);
        ConfigGlobal[toks[0]] = toks[1];
    }
}

string GetConfig(const string& key, const string& defval) {
    if (!ConfigGlobal.count(key)) return defval;
    return ConfigGlobal[key];
}
int GetConfig(const string& key, int defval) {
    if (!ConfigGlobal.count(key)) return defval;
    return atoi(ConfigGlobal[key].c_str());
}
double GetConfig(const string& key, double defval) {
    if (!ConfigGlobal.count(key)) return defval;
    return atof(ConfigGlobal[key].c_str());
}