#!/bin/bash
# ==============================================================================
# Cobalt Coin: Master Ensemble Production (Co-60 100M, Cosmic 10M)
# ==============================================================================

# ==============================================================================
# [1] 마스터 파라미터 제어소 (이곳만 바꾸면 C++ 수정 없이 모든 실험이 가능합니다)
# ==============================================================================
THREADS=30
N_RUNS=10                # 10세트 반복 앙상블

CO60_EVENTS=10000000     # Co-60: 런당 1,000만 (총 1억 / 100M)
COSMIC_EVENTS=1000000    # Cosmic: 런당 100만 (총 1,000만 / 10M)

DETECTOR_DISTANCE=13     # 선원-검출기 거리 (cm)
BASE_RADIUS=25           # 검출기 기본 반지름 (mm) -> 25mm = 2인치 PMT 세팅

START_ANGLE=30
END_ANGLE=180
STEP_ANGLE=10

echo "================================================================="
echo " 🚀 [마스터 프로덕션] 가변 기하학 앙상블 런을 시작합니다"
echo " 거리: ${DETECTOR_DISTANCE}cm | 반경: ${BASE_RADIUS}mm | 스레드: ${THREADS}"
echo " Co-60 통계: ${CO60_EVENTS} x ${N_RUNS} = 총 100M"
echo " Cosmic 통계: ${COSMIC_EVENTS} x ${N_RUNS} = 총 10M"
echo "================================================================="

for angle in $(seq ${START_ANGLE} ${STEP_ANGLE} ${END_ANGLE}); do

    echo -e "\n================================================================="
    echo " >>> [진행중] 현재 각도: ${angle} 도 스캔 시작"
    echo "================================================================="

    for run_id in $(seq 1 ${N_RUNS}); do
        echo "   -> [${angle}도] Run ${run_id} / ${N_RUNS} 진행 중..."

        # ===================================================================
        # [2] Co-60 (시그널) 매크로 동적 생성
        # ===================================================================
        cat << EOF > temp_co60.mac
/run/numberOfThreads ${THREADS}
/run/initialize
/process/had/rdm/thresholdForVeryLongDecayTime 1.0e+9 y

/myApp/tracking/storeOpticalTraj false

/gps/particle ion
/gps/ion 27 60 0 0
/gps/energy 0. keV
/gps/pos/type Point
/gps/pos/centre 0. 0. 0. cm
/gps/ang/type iso

/myApp/daq/setThreshold 5
/myApp/daq/setCoinWindow 100 ns
/myApp/daq/setPileUpWindow 50 ns

# ★ 동적 기하학 제어 매크로 주입 (거리, 반경, 각도)
/myApp/detector/setDistance ${DETECTOR_DISTANCE} cm
/myApp/detector/setBaseRadius ${BASE_RADIUS} mm
/myApp/detector/setMovableAngle ${angle} deg

/myApp/daq/setCurrentAngle ${angle} deg
/myApp/daq/setSourceType 1

/analysis/setFileName Co60_10M_Angle_${angle}_Run_${run_id}.root

/run/printProgress 1000000
/run/beamOn ${CO60_EVENTS}
EOF

        ./Co60Sim temp_co60.mac > /dev/null 2>&1

        # ===================================================================
        # [3] Cosmic (백그라운드) 매크로 동적 생성
        # ===================================================================
        cat << EOF > temp_cosmic.mac
/run/numberOfThreads ${THREADS}
/run/initialize

/myApp/tracking/storeOpticalTraj false

/myApp/daq/setThreshold 5
/myApp/daq/setCoinWindow 100 ns
/myApp/daq/setPileUpWindow 50 ns

# ★ 백그라운드 런에도 동일한 기하학 제어 매크로 주입 필수!
/myApp/detector/setDistance ${DETECTOR_DISTANCE} cm
/myApp/detector/setBaseRadius ${BASE_RADIUS} mm
/myApp/detector/setMovableAngle ${angle} deg

/myApp/daq/setCurrentAngle ${angle} deg
/myApp/daq/setSourceType 0

/analysis/setFileName Cosmic_1M_Angle_${angle}_Run_${run_id}.root

/run/printProgress 500000
/run/beamOn ${COSMIC_EVENTS}
EOF

        ./Co60Sim temp_cosmic.mac > /dev/null 2>&1

    done
done

# 임시 매크로 정리
rm -f temp_co60.mac temp_cosmic.mac

echo -e "\n🎉 [종료] C++ 동적 제어를 활용한 대규모 프로덕션(110M)이 완벽하게 완료되었습니다!"
