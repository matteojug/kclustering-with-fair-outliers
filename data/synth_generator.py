# To generate a test instance, use the same args.seed_coeff as the training instance

import numpy as np
import argparse, sys, math
import matplotlib.pyplot as plt
from common import *

parser = argparse.ArgumentParser()
parser.add_argument('--k', help='clusters', type=int, default=5)
parser.add_argument('--n', help='n', type=int, default=5000)
parser.add_argument('--pmain', help='n_mainstream', type=float, default=0)
parser.add_argument('--pout', help='n_minory', type=float, default=0)
parser.add_argument('--seed', help='seed', default=42, type=int)
parser.add_argument('--seed-coeff', help='seed-coeff', default=None, type=int)
parser.add_argument('--path', help='saving path', default="synth.ds")
parser.add_argument('--plot', help='save plot', action="store_true")
parser.add_argument('--plot-show', help='show plot', action="store_true")
args = parser.parse_args()
need_plot = args.plot_show or args.plot
if args.seed_coeff is None: args.seed_coeff = args.seed

meta = {"generator":" ".join(sys.argv), "arg.seed": str(args.seed), "arg.seed_coeff": str(args.seed_coeff)}
print(meta)

def foo(k, n, p_main, p_out, scale=10, seed=42, seed_coeff=42):
    rnd = np.random.RandomState(seed)
    pts = []
    colors = []
    clusters = []
    
    n_main = int(n*p_main/(k-1))
    n_out = int(n*p_out)
    n_min = n-(n_main*(k-1)+n_out)
    print(n_main, n_min, n_out)
    
    n_colors = k
    sigma_cluster = k*3
    
    # sample 2 normali indipendenti per i coefficenti, moltiplica per le feature + gaussian noise
    # generate also a test instance with the same centers
    for i in range(k):
        center = np.array([math.sin(2*math.pi*i/k),math.cos(2*math.pi*i/k)])*scale*k*2
        if i > 0:
            pts.extend(rnd.normal(loc=center, scale=sigma_cluster, size=[n_main,2]))
            colors.extend(rnd.randint(0,n_colors-1,size=[n_main]))
            clusters.extend([i]*n_main)
        else:
            pts.extend(rnd.normal(loc=center, scale=sigma_cluster, size=[n_min,2]))
            colors.extend([n_colors-1]*n_min)
            clusters.extend([i]*n_min)
    
    p = [0]*n_colors
    for c in colors: p[c] += 1
    pts.extend(rnd.uniform(low=-scale*k*1000, high=+scale*k*1000, size=[n_out,2]))
    colors.extend(rnd.choice(n_colors, n_out, p=[x/len(colors) for x in p]))
    clusters.extend([-1]*n_out)
    
    rnd_coeff = np.random.RandomState(seed_coeff)
    coeffs = {k:list(rnd_coeff.normal(size=[2])) for k in [*range(k)]+[-1]}
    labels = []
    for i in range(len(pts)):
      # using rnd.normal for noise to avoid same sequence noise for test
      labels.append(sum(map(lambda x: pts[i][x]*coeffs[clusters[i]][x], range(2)))+rnd.normal())

    t = [*zip(pts, colors, clusters, labels)]
    rnd.shuffle(t)
    pts, colors, clusters, labels = zip(*t)

    return pts, colors, clusters, labels, coeffs

X, c, cluster_id, labels, coeffs = foo(k=args.k, n=args.n, p_main=args.pmain, p_out=args.pout, seed=args.seed, seed_coeff=args.seed_coeff)
meta["cluster_id"] = " ".join(map(str,cluster_id))

dataset = Dataset("synth", ["dummy"])
pt_to_label = {}
for i, (x,c) in enumerate(zip(X,c)):
  p = Point(i, x, [str(c)])
  dataset.points.append(p)
  pt_to_label[p.id] = labels[i]

if need_plot:
  cmap, cmap_dom = "tab10", 10
  fig, axs = plt.subplots(3, figsize=(6,10))
  for i in range(2):
    axs[i].scatter(*zip(*[p.x[:2] for p in dataset.points]), s=10, alpha=0.2, c=[int(p.colors[0]) for p in dataset.points], vmin=0, vmax=cmap_dom-1, cmap=cmap)
  axs[1].set_yscale('symlog')
  axs[1].set_xscale('symlog')
  
  axs[2].scatter(*zip(*[p.x[:2] for p in dataset.points if cluster_id[p.id] != -1]), s=10, alpha=0.2, c=[int(p.colors[0]) for p in dataset.points if cluster_id[p.id] != -1], vmin=0, vmax=cmap_dom-1, cmap=cmap)
  fig.tight_layout()

dataset.denumpy()
dataset.info()
dataset.dump(args.path, meta=meta)
dataset.meta = meta
d = Dataset.load(args.path)
assert(dataset == d)

import json
with open(args.path+".labels", 'w') as f:
    json.dump({"labels": pt_to_label, "coeffs": coeffs}, f)

if args.plot:
  plt.savefig(args.path+".png", dpi=300)    
if args.plot_show:
  plt.show()
