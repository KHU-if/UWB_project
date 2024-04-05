import math

def calculate_centroid(p1, p2, p3):
    # p1, p2, p3는 삼각형의 세 꼭지점을 나타내는 좌표입니다.
    x1, y1 = p1[0], p1[1]
    x2, y2 = p2[0], p2[1]
    x3, y3 = p3[0], p3[1]

    # 무게 중심을 계산하기 위해 각 꼭지점의 x 및 y 좌표를 더합니다.
    centroid_x = (x1 + x2 + x3) / 3
    centroid_y = (y1 + y2 + y3) / 3

    return centroid_x, centroid_y

def get_intersections(x0, y0, r0, x1, y1, r1):
    # circle 1: (x0, y0), radius r0
    # circle 2: (x1, y1), radius r1

    d=math.sqrt((x1-x0)**2 + (y1-y0)**2)

    # non intersecting
    if d > r0 + r1 :
        return []
    # One circle within other
    if d < abs(r0-r1):
        return []
    # coincident circles
    if d == 0 and r0 == r1:
        return []
    else:
        a=(r0**2-r1**2+d**2)/(2*d)
        h=math.sqrt(r0**2-a**2)
        x2=x0+a*(x1-x0)/d   
        y2=y0+a*(y1-y0)/d   
        x3=x2+h*(y1-y0)/d     
        y3=y2-h*(x1-x0)/d 
        x4=x2-h*(y1-y0)/d
        y4=y2+h*(x1-x0)/d
        return [[round(x3,2), round(y3,2)], [round(x4,2), round(y4,2)]]

def dist(p1, p2):
    return round(math.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2), 2)

def from_car_user_location(m1, m2, m3, r1, r2, r3):
    intersection_sets = [
        get_intersections(m1[0], m1[1], r1, m2[0], m2[1], r2),
        get_intersections(m1[0], m1[1], r1, m3[0], m3[1], r3),
        get_intersections(m2[0], m2[1], r2, m3[0], m3[1], r3)
    ]
    intersections = [point for subset in intersection_sets for point in subset if len(subset) > 0]
    
    if not intersections:
        return None  # 교차점이 없으면 None 반환

    c = 0.05
    user_locs = []

    while not user_locs and c <= 1:  # c 값이 1을 초과하지 않도록 하여 무한 루프 방지
        threshold = (r1 + r2 + r3) / 3 * c
        for i, point1 in enumerate(intersections):
            for point2 in intersections[i+1:]:
                if dist(point1, point2) <= threshold:
                    user_locs.append(point1)
                    user_locs.append(point2)
        c += 0.01  # c 값을 증가시키며 임계값 조정

        # 유니크한 교차점만 유지
        user_locs = list(set(tuple(loc) for loc in user_locs))
    
    if not user_locs:
        return None  # 교차점이 없으면 None 반환
    
    
    # 교차점들의 평균 위치 계산
    avg_x = sum(point[0] for point in user_locs) / len(user_locs)
    avg_y = sum(point[1] for point in user_locs) / len(user_locs)
    return [round(avg_x, 2), round(avg_y, 2)]