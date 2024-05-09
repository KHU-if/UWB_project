import requests
import time
import math

diff = 0.34 # 앞의 두 앵커 사이의 거리

def GetXY(a, b):
    """
    a is smaller then b
    if a, b has no joint, return None
    """
    if a + diff <= b:
        return None
    x = (b ** 2 - a ** 2 - diff ** 2) / (2 * diff)
    y = (a ** 2 - x ** 2) ** 0.5
    x += diff / 2
    return x, y

while True:
    time.sleep(0.4)
    r = requests.get('http://192.168.0.15/')
    if r.status_code != 200:
        print('fail request')
        continue
    data = r.text
    data = data.split('/')
    data = list(map(float, data))
    print(data)

    a = data[1] - 1
    b = data[2] - 1
    alist.append(a)
    blist.append(b)
    if (len(alist) > 10):
        alist.pop(0)
    if (len(blist) > 10):
        blist.pop(0)
    #a = sum(alist) / len(alist)
    #b = sum(blist) / len(blist)

    rest = []
    da = -0.1
    while da <= 0.1:
        db = -0.1
        while db <= 0.1:
            swap = False
            ta = a + da
            tb = b + db
            if ta > tb:
                ta, tb = tb, ta
                swap = True
            res = GetXY(ta, tb)
            if res != None:
                x, y = res
                if swap:
                    x = -x
                rest.append((x, y, da, db))

            db += 0.01
        da += 0.01
    
    print('len', len(rest))
    if len(rest) == 0:
        print('no point detected')
        continue
    resx = 0
    resy = 0
    weightsum = 0
    for p in rest:
        diff = (0.11 - abs(p[2])) * 100 * (0.11 - abs(p[3])) * 100
        resx += p[0] * diff
        resy += p[1] * diff
        weightsum += diff
    print('a', a, 'b', b)
    print('res', resx / weightsum, resy / weightsum)
    rad = math.atan(resy / resx)
    if rad < 0:
        rad += 3.14159265
    print('rad', rad * 180 / 3.14159265)
