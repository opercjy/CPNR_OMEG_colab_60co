# CPNR_OMEG_colab_60co
# Cobalt Coin: Co-60 Angular Correlation & Cosmic-Ray Coincidence Simulation

[![Geant4](https://img.shields.io/badge/Geant4-v11.x-blue.svg)](https://geant4.web.cern.ch/)
[![ROOT](https://img.shields.io/badge/ROOT-v6.3x-blue.svg)](https://root.cern/)
[![CRY](https://img.shields.io/badge/CRY-v1.7-darkred.svg)](https://nuclear.llnl.gov/simulation/)
[![OS](https://img.shields.io/badge/OS-Rocky%20Linux%209-green.svg)](https://rockylinux.org/)

본 프로젝트는 **Geant4 11.x** 및 **CRY(Cosmic-ray Shower Library)**를 기반으로 구축된 정밀 입자 물리 전산모사 툴킷입니다. 두 개의 액체 섬광체(Liquid Scintillator) 모듈을 이용한 **Co-60 감마선 캐스케이드(Cascade) 각상관(Angular Correlation) 측정** 및 **우주선(Cosmic-ray Muon) 관통 백그라운드 Veto 시스템**을 모사하고 분석합니다.

---

## 1. Physics & Mathematical Foundation (물리학적·수학적 기반)

### 1.1. Co-60 Correlated Gamma Emission
Co-60의 방사성 붕괴 시 발생하는 1.17 MeV와 1.33 MeV 감마선은 독립적이지 않고 양자역학적인 각상관(Angular Correlation)을 갖습니다. 본 시뮬레이션은 `G4RadioactiveDecayPhysics`와 `G4DeexPrecoParameters::SetCorrelatedGamma(true)`를 활성화하여, 두 감마선이 방출될 때의 물리적 방향성 비대칭(Anisotropy)을 커널 레벨에서 완벽히 모사합니다.

### 1.2. Optical Physics & Boundary Quantum Efficiency (QE)
PMT(광전증폭관)의 양자 효율(QE, 25% @ 400nm)을 처리하기 위해 재래식 난수 굴리기 로직을 폐기하고, Geant4 11.x의 표준인 **`SetBoundaryInvokeSD(true)`** 아키텍처를 도입했습니다. 
* 빛이 `dielectric_metal` 경계면에 도달하면 `G4OpBoundaryProcess`가 자체적으로 25%의 검출 확률(Detection Probability)을 계산합니다.
* 확률 주사위를 통과한 진짜 '광전자(Photoelectron)'들만 `PMTSD` (Sensitive Detector)로 보고되어 계측되므로, 물리적 무결성이 100% 보장됩니다.

### 1.3. Convolution of Energy Resolution (에너지 분해능 합성곱)
검출기에서 측정된 최종 에너지 스펙트럼 $E_{reco}$는 액체 섬광체에 침적된 순수 에너지 $E_{true}$와 PMT 광학 수율의 포아송/가우시안 확률 밀도 함수(PDF)가 합성곱(Convolution)된 결과입니다.
$$E_{reco} \approx E_{true} \ast \text{Poisson}(\mu)$$
본 시뮬레이션은 `SteppingAction`을 통해 $E_{true}$ (순수 침적 에너지)를 별도로 획득하고, 가상 DAQ를 통해 $E_{reco}$ (광전자 변환 에너지)를 기록하여, 분석 단계에서 이 두 스펙트럼의 합성곱 관계를 면적 정규화(Area Normalization)를 통해 수학적으로 증명합니다.

---

## 2. Computational Architecture (전산학적 최적화 설계)

대규모 몬테카를로(1,000만 런 이상) 시뮬레이션 시 필연적으로 발생하는 **메모리 붕괴(OOM)** 및 **멀티스레딩(MT) 병목**을 방어하기 위해 설계된 CS 아키텍처입니다.

* **O(1) Memory Pool & Accumulative Hit:** 매번 광자가 발생할 때마다 동적 할당(`new PMTHit`)을 수행하면 힙(Heap) 메모리 단편화가 발생합니다. 이를 막기 위해 이벤트 시작 시 모듈당 1개의 빈 객체만 미리 할당하고, 광자가 닿을 때마다 `PE++`로 정수 값만 누적시키는 **O(1) 아키텍처**를 구현했습니다.
* **Tracking Pruning (메모리 암살자):** 우주선 샤워 시 발생하는 수억 개의 광학 광자 궤적(Trajectory)이 RAM을 점유하지 않도록, `TrackingAction`에서 광자의 `StoreTrajectory` 플래그를 커스텀 UI 메신저를 통해 동적으로 차단합니다.
* **Bypass Routing in SteppingAction:** 시뮬레이션 스텝의 99%를 차지하는 '에너지 손실 없는 광자의 이동'을 문자열 판별(String matching) 이전에 `if (edep <= 0.0) return;` 단 한 줄로 바이패스(Bypass)하여 CPU 캐시 미스를 극적으로 방어했습니다.
* **Thread-safe Dependency Injection:** MT 환경에서의 Data Race를 막기 위해, `ActionInitialization`에서 `EventAction` 포인터를 생성한 뒤 이를 `SteppingAction`에 의존성 주입(DI)하여 스레드-로컬(Thread-local) 통신을 구현했습니다.

---

## 3. Detector Geometry & Material Modeling (기하 구조 및 물성)

입자의 상호작용 및 빛의 난반사 통로를 완벽하게 제어하기 위해 모든 부품의 물성이 실제 실험 환경에 맞게 조율되었습니다.

* **Target Material (LS):** LAB + PPO 기반 액체 섬광체. (발광량: 10,000 / MeV)
* **Optical Window & Coupling:** 듀란병(Pyrex Glass, $n=1.47$)과 광학 그리스(Silicon Dioxide, $n=1.47$)의 굴절률(Index) 매칭을 통해 프레넬 반사(Fresnel Reflection) 손실을 억제.
* **Reflector (Tyvek & Al):** `UNIFIED` 모델의 `groundfrontpainted` 속성을 적용. 98%의 람베르트 난반사(Lambertian Diffuse Reflection)를 강제하여 빛의 집광 수율을 현실적으로 모사.
* **Dead Materials:** 3D 프린터 지지대(PETG), 연납 차폐체(Pb), 알루미늄 하우징(Al)에 침적되는 순수 에너지를 별도로 스코어링하여 컴프턴 백스캐터링 및 관통 우주선의 Veto 효율을 평가.

---

## 4. Source Tree Structure (디렉토리 구조)

```text
cobalt_coin/
├── CMakeLists.txt          # CMake 빌드 스크립트 (C++17 표준)
├── main.cc                 # Geant4 메인 진입점 및 MT 런매니저
│
├── include/                # C++ 헤더 파일
│   ├── ActionInitialization.hh
│   ├── DetectorConstruction.hh   # 3차원 적층형 검출기 및 광학 표면
│   ├── EventAction.hh            # PE 합산 및 순수 침적 에너지 버퍼
│   ├── MyPhysicsList.hh          # Optical, Decay, RDM 물리 리스트
│   ├── PMTHit.hh / PMTSD.hh      # O(1) 메모리 풀 및 SD(Sensitive Detector)
│   ├── PrimaryGeneratorAction.hh # GPS(Co60) 및 CRY(Cosmic) 분리 생성기
│   ├── RunAction.hh              # ROOT Ntuple IO 및 MT 메신저 래퍼
│   ├── SteppingAction.hh         # Dead Material 초고속 에너지 라우팅
│   └── TrackingAction.hh         # 광학 광자 궤적 동적 제어 (OOM 방어)
│
├── src/                    # C++ 소스 파일 (구현부)
│   └── *.cc
│
└── macros/                 # 런타임 제어 및 파이썬 분석 스크립트
    ├── init_vis.mac              # 시각화(GUI) 초기화
    ├── run_co60.mac              # Co-60 시그널 대규모 런 생성
    ├── run_cosmic.mac            # CRY 우주선 백그라운드 런 생성
    ├── scan_all.mac / run_single # 각도별 (30~180도) 자동화 스캔
    ├── plot_resolution.py        # 분해능 합성곱 및 Veto 에너지 시각화 (Python)
    └── analysis_coincidence.py   # 가상 DAQ 타임-스티칭 및 2x2 코인시던스 시각화
```

---

## 5. Docker Environment & Build (도커 환경 및 빌드)

운영체제(OS) 환경에 종속되지 않는 재현성을 위해 **Rocky Linux 9** 기반의 도커 컨테이너 환경에서 빌드하는 것을 권장합니다. (Geant4 11.2+, ROOT 6.30+, CRY 1.7+ 포함 필요)

```bash
# 1. 빌드 디렉토리 생성 및 CMake 구성
mkdir build && cd build
cmake ..

# 2. 멀티코어 병렬 컴파일 (속도 극대화)
make -j$(nproc)

# 3. 대화형 시각화(GUI) 실행
./Co60Sim

# 4. 배치 모드 (대규모 데이터 생성)
./Co60Sim macros/run_co60.mac    # Co-60 캐스케이드 데이터 생성
./Co60Sim macros/run_cosmic.mac  # 우주선 관통 데이터 생성
```

---

## 6. Data Analysis Pipeline (분석 파이프라인)

본 툴킷은 `uproot`과 `mplhep` (CERN 표준 스타일) 라이브러리를 활용한 파이썬 기반의 가상 DAQ(Toy MC) 스크립트를 내장하고 있습니다. 시뮬레이션 직후 `build` 폴더 내에서 곧바로 실행할 수 있습니다.

### 6.1. 분해능 및 데드 머터리얼 분석
```bash
python3 plot_resolution.py
```
* **기능:** 순수 침적 에너지(True Edep)와 광전자 스펙트럼(PE)을 정규화하여 겹쳐 그림으로써, 검출기 분해능에 의한 뭉개짐(Broadening) 현상을 증명합니다. 납(Pb)이나 알루미늄(Al)에 침적된 에너지를 분석하여 우주선 Veto 컷의 최적값을 도출할 수 있습니다.

### 6.2. 가상 DAQ 동시계수 (Time-Stitched Coincidence) 분석
```bash
python3 analysis_coincidence.py
```
* **기능:** Co-60 런과 우주선 런의 데이터를 읽어 들여 푸아송 분포 기반의 절대 시간(Global Time)을 부여한 뒤 타임라인을 병합(Stitching)합니다.
* 설정된 동시계수 윈도우(`CoinWindow = 100ns`) 안에서 발생하는 '진짜 동시계수(True Coincidence)'와 '우연 동시계수(Accidental)'를 분리해내며, 궁극적으로 1.17 MeV와 1.33 MeV가 더해진 **2.50 MeV Sum Peak**를 2x2 상관관계 플롯으로 시각화합니다.
