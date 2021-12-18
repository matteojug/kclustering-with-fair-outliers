import numpy as np
import random
from dataclasses import dataclass, field
from typing import List
from collections import defaultdict
import re

@dataclass
class Point:
    id: int
    x: List[float]
    colors: List[str] = field(default_factory=list)

@dataclass
class Dataset:
    name: str = "???"
    colors_header: List[str] = field(default_factory=list)
    points: List[Point] = field(default_factory=list)
    meta: dict = field(default_factory=dict)

    def init_index(self):
        self.points_index = {}
        for i,pt in enumerate(self.points):
            self.points_index[pt.id] = i

        self.points_cluster = {}
        if "cluster_id" in self.meta:
            for i,cid in enumerate(self.meta["cluster_id"].split()):
                self.points_cluster[self.points[i].id] = int(cid)
        else:
            for k in self.points_index:
                self.points_cluster[k] = -2
                
        self.init_color_count()

    def init_color_count(self):
        self.color_count = []
        for i,c in enumerate(self.colors_header):
            k = defaultdict(int)
            for p in self.points:
                k[p.colors[i]] += 1
            self.color_count.append(dict(k))
    
    def point_by_id(self, id):
        return self.points[self.points_index[id]]

    def points_id(self):
        for pt in self.points:
            yield pt.id
    
    def dimension(self):
        dim = set(map(lambda x: len(x.x), self.points))
        assert(len(dim) == 1)
        return list(dim)[0]
    
    def denumpy(self):
        for i,p in enumerate(self.points):
            p.x = list(p.x)

    def info(self):
        print(f"# {self.name}")
        print(f"Size: {len(self.points)}, dim: {self.dimension()}")
        print(f"Colors: {len(self.colors_header)}")
        for i,c in enumerate(self.colors_header):
            k = defaultdict(int)
            for p in self.points:
                k[p.colors[i]] += 1
            print(f"[{i}] {c}({len(k)}): {dict(k)}")

    def prune(self, to, seed):
        random.Random(seed).shuffle(self.points)
        self.points = self.points[:to]
    
    @staticmethod
    def load(path):
        d = Dataset()
        with open(path) as fin:
            line = "#"
            while line.startswith("#"):
                if line.startswith("#@"):
                    s = line[2:].rstrip().split(":", 1)
                    d.meta[s[0]] = s[1]
                line = fin.readline()
            d.name, points_size, dim = line.split()
            points_size = int(points_size)
            dim = int(dim)
            color_size, *d.colors_header = fin.readline().split()
            color_size = int(color_size)
            assert(len(d.colors_header) == color_size)
            for line in fin:
                h, *t = line.split()
                assert(len(t) == dim+color_size)
                pt = Point(int(h), [*map(float,t[:dim])], t[dim:])
                d.points.append(pt)
            assert(d.dimension() == dim)
            assert(len(d.points) == points_size)
        return d
    
    def dump(self, path, comment=None, meta=None):
        assert(is_safe(self.name))
        assert(len(set(map(lambda x: x.id, self.points))) == len(self.points))
        assert(all(map(is_safe, self.colors_header)))
        for p in self.points:
            assert(len(self.colors_header) == len(p.colors))
            assert(all(map(is_safe, p.colors)))

        with open(path, "w") as fout:
            if comment is not None:
                for c in comment.split("\n"):
                    fout.write(f"# {c}\n")
            if meta is not None:
                for k,v in meta.items():
                    assert("\n" not in v)
                    fout.write(f"#@{k}:{v}\n")
            fout.write(f"{self.name} {len(self.points)} {self.dimension()}\n")
            fout.write(f"{len(self.colors_header)} ")
            fout.write(" ".join(self.colors_header)+"\n")
            for p in self.points:
                fout.write(f"{p.id} ")
                fout.write(" ".join(map(str,p.x))+" ")
                fout.write(" ".join(map(str,p.colors))+"\n")


def is_safe(v):
    return re.match("[\w._-]+$", str(v))

def normalize(vecs):
    return (vecs-np.mean(vecs,0))/np.std(vecs,0)