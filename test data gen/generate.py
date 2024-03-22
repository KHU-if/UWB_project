import random

# basic params
anc1 = (0, 0, 0)
anc2 = (1, 0, 0)
anc3 = (0, 1, 0)

def dist(p1, p2):
    """
    euclidean distance of 2 3d points
    params: p1, p2: tuple(float, float, float)
    return: dist: float
    """
    distsq = (p1[0] - p2[0]) ** 2 + (p1[1] - p2[1]) ** 2 + (p1[2] - p2[2]) ** 2
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
    d1 = noise(dist(p1, p))
    d2 = noise(dist(p2, p))
    d3 = noise(dist(p3, p))
    return alldist(d1, d2, d3)

def makep(mx, Mx, my, My, mz, Mz):
    """
    make random tag position
    """
    dx = Mx - mx
    dy = My - my
    dz = Mz - mz
    return (mx + random.random() * dx, my + random.random() * dy, mz + random.random() * dz)

def evaluate(calcfunc):
    rp = makep(10, 30, 10, 30, 0, 10)
    dists = alldist(p)
    ep = calcfunc(dists[0], dists[1], dists[2])
    print("rp", rp)
    print("ep", ep)
    print("diff", dist(rp, ep))

# call evaluate function with made point calc function