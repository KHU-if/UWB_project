import math
import functions

def main(m1,m2,m3,r1,r2,r3):
    
    # User 기준 자동차 중심의 위치
    loc_from_user = [-1 * functions.from_car_user_location(m1, m2, m3, r1, r2, r3)[0], -1 * functions.from_car_user_location(m1, m2, m3, r1, r2, r3)[1]]

    # 각도 계산
    theta = math.atan(loc_from_user[1]/loc_from_user[0])*180/math.pi

    if loc_from_user[0]>=0 and loc_from_user[1]>=0:
        degree = theta
    elif loc_from_user[0]<0 and loc_from_user[1]>=0:
        degree = 180-theta
    elif loc_from_user[0]<0 and loc_from_user[1]<0:
        degree = 180+theta
    elif loc_from_user[0]>=0 and loc_from_user[1]<0:
        degree = 360-theta


    print('거리: ',functions.dist(loc_from_user,[0,0]), 'm')
    print('각도: ', degree)
    return loc_from_user


user = [2,641]


# m1, m2, m3는 차의 중심을 기준으로 모듈들이 얼마만큼(m 단위) 떨어져 있는지
m1 = [0,0.5]
m2 = [-1,-0.5]
m3 = [1,-0.5]

# r1, r2, r3는 uwb 모듈 연계 이후 time to distance 로 바꿀 예정
r1 = functions.dist(m1, user)
r2 = functions.dist(m2, user)
r3 = functions.dist(m3, user)

if __name__=="__main__":
    main(m1,m2,m3,r1,r2,r3)