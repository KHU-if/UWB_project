import random

# basic params
anc1 = [0,0.16]
anc2 = [0.16,0]
anc3 = [-0.16,0]

def dist(p1, p2):
    """
    euclidean distance of 2 3d points
    params: p1, p2: tuple(float, float, float)
    return: dist: float
    """
    distsq = (p1[0] - p2[0]) ** 2 + (p1[1] - p2[1]) ** 2
    return distsq ** 0.5

def noise(d):
    """
    apply error
    """
    d += 0.6 + random.random() * 0.2
    return d

def alldist(p):
    """
    simulate dist from tree anchors
    """
    d1 = noise(dist(anc1, p))
    d2 = noise(dist(anc2, p))
    d3 = noise(dist(anc3, p))
    return (d1, d2, d3)

def makep(mx, Mx, my, My):
    """
    make random tag position
    """
    dx = Mx - mx
    dy = My - my
    return (mx + random.random() * dx, my + random.random() * dy)

def evaluate(calcfunc):
    rp = makep(10, 30, 10, 30, 0, 10)
    dists = alldist(p)
    ep = calcfunc(dists[0], dists[1], dists[2])
    print("rp", rp)
    print("ep", ep)
    print("diff", dist(rp, ep))

# call evaluate function with made point calc function

import main

data = [
    [3.6, 3.6, 3.72],
    [4.11, 4.15, 4.05],
    [2.8, 2.57, 2.61],
    [2.44, 2.43, 2.71],
    [2.45, 2.77, 2.25],
    [3.65, 4.58, 3.43],
    [3.55, 3.79, 3.65],
    [3.77, 4.11, 3.57]
]

for d in data:
    print(d)
    cnt = 0
    res = None
    while res == None and cnt < 10:
        try:
            res = main.main(anc1, anc2, anc3, d[0] - 0.8, d[1] - 0.8, d[2] - 0.8)
        except:
            pass
        cnt += 1
    print(res)
    

"""
user = makep(10, 50, 10, 50)

print(user)

d1, d2, d3 = alldist(user)
print(d1, d2, d3)
d1 -= 0.7
d2 -= 0.7
d3 -= 0.7

data = None
cnt = 0
while data == None and cnt < 10:
    try:
        data = main.main(anc1, anc2, anc3, d1, d2, d3)
    except:
        pass
    cnt += 1
print(data)
"""