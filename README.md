# Cobalt Coin: Co-60 Angular Correlation & Cosmic-Ray Coincidence Simulation

[![Geant4](https://img.shields.io/badge/Geant4-v11.x-blue.svg)](https://geant4.web.cern.ch/)
[![ROOT](https://img.shields.io/badge/ROOT-v6.3x-blue.svg)](https://root.cern/)
[![CRY](https://img.shields.io/badge/CRY-v1.7-darkred.svg)](https://nuclear.llnl.gov/simulation/)
[![OS](https://img.shields.io/badge/OS-Rocky%20Linux%209-green.svg)](https://rockylinux.org/)

본 프로젝트는 **Geant4 11.x** 및 \*\*CRY(Cosmic-ray Shower Library)\*\*를 기반으로 구축된 정밀 입자 물리 전산모사 툴킷입니다. 두 개의 액체 섬광체(Liquid Scintillator) 모듈을 이용한 **Co-60 감마선 캐스케이드(Cascade) 각상관(Angular Correlation) 측정** 및 **우주선(Cosmic-ray Muon) 관통 백그라운드 Veto 시스템**을 모사하고 분석합니다.

-----

## 1\. Overview & Design Philosophy (설계 철학 및 당위성)

본 시스템은 초저방사능(Ultra-low Background) 실험의 성능 한계를 극복하기 위해, 일반적인 시뮬레이션 예제 수준을 벗어난 고도의 아키텍처가 적용되었습니다. 초보 연구원 및 동료들을 위해 본 시스템에 적용된 \*\*기술적 당위성(Why we do this)\*\*을 먼저 브리핑합니다.

  * **왜 도커(Docker)를 사용하는가?**
    Geant4와 ROOT, CRY 라이브러리는 운영체제 및 C++ 표준(C++17) 의존성이 매우 높습니다. 로컬 PC에 직접 설치할 경우 발생하는 수많은 라이브러리 충돌 에러를 원천 차단하기 위해, 전 세계 어디서든 100% 동일한 런타임과 연구 환경(VS Code, Neovim 등)을 보장하는 도커 컨테이너를 사용합니다.
  * **왜 광학 물리(Optical Physics)는 켜두고 궤적(Trajectory) 저장만 차단하는가?**
    섬광체 내에서 발생한 광자가 테플론의 람베르트 난반사를 거쳐 PMT 광음극에 도달하는 미시적 상호작용은 검출기의 집광 효율을 결정짓는 핵심입니다. 이를 단순 역산으로 퉁치면 물리적 무결성이 훼손됩니다. 따라서 **광학 물리 엔진은 100% 풀가동**하여 빛의 흐름을 완벽히 모사하되, 수백억 개의 광자 궤적 데이터를 RAM에 기록하다가 시스템이 붕괴(OOM)되는 것을 막기 위해 `TrackingAction`과 커스텀 UI 커맨드를 도입, \*\*메모리 할당만 선택적으로 차단(Pruning)\*\*하는 아키텍처를 채택했습니다.
  * **왜 스코어링(Scoring Mesh) 대신 SD(Sensitive Detector)를 직접 구현했는가?**
    PMT 표면은 에너지가 쌓이는 곳이 아니라 **광학 광자(Optical Photon)가 광전자(Photoelectron)로 변환되는 곳**입니다. 커널에서 `SetBoundaryInvokeSD(true)`를 활성화하여, Geant4가 PMT 표면(`dielectric_metal`)에 도달한 광자에 대해 25% 양자 효율(QE) 주사위를 먼저 굴리게 합니다. 이 물리적 확률 연산을 통과한 "진짜 광전자"들만 SD로 보고되게 하여 시뮬레이션의 양자역학적 신뢰도를 극대화했습니다.

-----

## 2\. Physics & Mathematical Foundation (물리학적·수학적 기반)

  * **Co-60 Correlated Gamma Emission:** Co-60 붕괴 시 발생하는 1.17 MeV와 1.33 MeV 감마선은 독립적이지 않고 공간적 각상관을 갖습니다. `G4DeexPrecoParameters::SetCorrelatedGamma(true)`를 활성화하여, 두 감마선 방출 시 물리적 방향성 비대칭(Anisotropy)을 커널 레벨에서 모사했습니다.
  * **Convolution of Energy Resolution (에너지 분해능 합성곱):** 검출기에서 측정된 최종 스펙트럼 $E_{reco}$는 섬광체에 침적된 순수 에너지 $E_{true}$에 PMT 광학 수율의 포아송 확률 밀도 함수(PDF)가 합성곱(Convolution)된 결과입니다.
    $$E_{reco} \approx E_{true} \ast \text{Poisson}(\mu)$$
    본 코드는 `SteppingAction`을 통해 $E_{true}$를 별도로 획득하고, 가상 DAQ를 통해 $E_{reco}$를 기록하여 두 스펙트럼의 합성곱 관계를 면적 정규화(Area Normalization)를 통해 수학적으로 분석합니다.

-----

## 3\. The Ultimate Docker Environment (컨테이너 구축 및 관리)

### 3.1. 이미지 빌드 (Build the Image)

저장소를 클론한 후, 최상위 디렉토리에서 아래 명령어로 도커 이미지를 생성합니다.

```bash
docker build -t my-g4-qt6:1.0 .
```

### 3.2. 컨테이너 자동화 실행 (Bash 환경 변수 등록)

Geant4의 GUI 시각화 창을 로컬 모니터로 포워딩하고 외부 데이터셋을 연동하기 위해, 매번 긴 명령어를 치는 대신 호스트 PC의 `~/.bashrc` (또는 `~/.zshrc`) 맨 끝에 아래의 쉘 함수를 추가하십시오.

```bash
# ~/.bashrc 파일 맨 아래에 추가
function rung4v2() {
  local image_name=${1:-"my-g4-qt6:1.0"}
  echo "Starting container with image: ${image_name}"

  # X11 접근 권한 허용 (GUI 렌더링 필수)
  xhost +local:docker >/dev/null 2>&1

  docker run -it --rm \
    --net=host \
    -e DISPLAY=$DISPLAY \
    -v $HOME/.Xauthority:/root/.Xauthority:ro \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    --name g4dev \
    -v "$(pwd)":/work \
    -w /work \
    -v /usr/local/share/ANNRI_Gd:/usr/local/share/ANNRI_Gd \
    "${image_name}" bash
}
```

터미널에서 `source ~/.bashrc` 실행 후, **`rung4v2`** 라고 입력하면 즉시 컨테이너 내부로 진입합니다.

#### [전문가 아키텍처 해설: 왜 이렇게 마운트하는가?]

  * `--net=host` 및 `X11 세팅`: 도커 내부의 Qt6 그래픽 신호를 로컬 호스트의 디스플레이 서버로 직결하여 Geant4 3D 뷰어와 ROOT TBrowser를 네이티브처럼 구동합니다.
  * **`ANNRI_Gd` 외부 볼륨 마운트:** 수 GB에 달하는 데이터셋을 도커 이미지에 직접 `COPY`하면 이미지가 비대해져 관리가 불가능해집니다. `ANNRI_Gd`는 J-PARC 시설에서 측정한 가돌리늄(Gd) 중성자 포획 정밀 평가 데이터(Evaluated Neutron Data)입니다. 차후 본 프로젝트를 **반-전자중성미자(IBD) 탐지나 열중성자 포획 시뮬레이션**으로 확장할 때, Geant4 기본 라이브러리를 로컬의 정밀 데이터로 덮어쓰기(Override) 위한 선제적 데이터 파이프라인 개통입니다. 설치 한 경우에만 설정, 안한 경우에는 주석 처리.

### 3.3. 도커 이미지 및 컨테이너 관리 명령어

디스크 용량 확보 및 관리를 위한 필수 명령어 모음입니다.

```bash
docker images               # 로컬에 빌드된 도커 이미지 목록 확인
docker ps                   # 현재 실행 중인 컨테이너 상태 확인
docker rmi my-g4-qt6:1.0    # 특정 도커 이미지 삭제
docker system prune -a      # [권장] 안 쓰는 더미 데이터(캐시, 컨테이너) 일괄 대청소
```

-----

## 4\. Computational Architecture Defense (전산학적 방어 논리)

대규모 몬테카를로 시뮬레이션 시 필연적으로 발생하는 전산학적 병목과 버그를 원천 차단했습니다.

  * **O(N) 오버헤드 99% 차단 및 Segfault 방어:** `SteppingAction` 최상단에 `if (edep <= 0.0) return;` 분기를 추가해, 시뮬레이션의 99%를 차지하는 광학 광자의 이동 스텝을 문자열 비교(String Matching) 이전에 즉시 바이패스(Bypass)시켜 CPU 캐시 미스를 방어했습니다. 또한 `GetHistoryDepth() < 1` 코드로 공기층 타격 시 터지는 Segfault를 차단했습니다.
  * **O(1) Memory Pool & Accumulative Hit:** 매번 광자가 발생할 때마다 힙(Heap) 동적 할당을 수행하는 대신, 이벤트 시작 시 모듈당 1개의 빈 객체만 미리 할당하고 `PE++`로 정수 값만 누적시키는 아키텍처를 구현했습니다.
  * **MT Data Race 및 UI 명령어 증발 차단:** 멀티스레드 환경에서 `TrackingAction`의 정적 플래그(Static flag) 제어를 워커 스레드가 아닌, 마스터 스레드에서만 인스턴스화되는 `RunAction`의 메신저를 통해 래핑(Wrapper/Delegate)하여 `Command Not Found` 에러와 스레드 충돌을 원천 차단했습니다.
  * **경계면 무한 루프(Step Size Artifact) 2중 방어:** 광자가 SD에 진입하여 계측된 직후, 또는 반사되지 못하고 흡수된 직후 `track->SetTrackStatus(fStopAndKill)`을 강제 호출하여, 경계면에서 무의미하게 맴도는(Trapping) 광자가 수학적으로 존재할 수 없게 설계했습니다.

-----

## 5\. Build & Execution (컴파일 및 실행)

`rung4v2` 명령어로 컨테이너에 진입한 후 아래 절차를 수행합니다.

```bash
# 1. 빌드 및 병렬 컴파일
mkdir build && cd build
cmake ..
make -j$(nproc)

# 2. [GUI 모드] 시각화 및 디버깅 (init_vis.mac 자동 로드)
./Co60Sim

# 3. [Batch 모드] 프로덕션 런 (100만+ 이벤트)
./Co60Sim macros/run_co60.mac    # Co-60 캐스케이드 동시계수 데이터 생성
./Co60Sim macros/run_cosmic.mac  # 우주선 관통 Veto 백그라운드 데이터 생성

# 4. [Auto Scan 모드] 각도별 각상관 스캔
./Co60Sim macros/scan_all.mac    # 30도 ~ 180도 자동 루프 실행
```

-----

## 6\. Data Analysis Pipeline (분석 파이프라인)

본 툴킷은 `uproot`과 `mplhep` 라이브러리를 활용한 파이썬 기반의 가상 DAQ(Toy MC) 스크립트를 내장하고 있습니다. (도커 환경 내장)

### 6.1. 분해능 및 데드 머터리얼 분석

```bash
python3 plot_resolution.py
```

  * 순수 침적 에너지(True Edep)와 광전자 스펙트럼(PE)을 정규화하여 겹쳐 그림으로써, 검출기 분해능에 의한 뭉개짐(Broadening) 현상을 증명합니다. 납(Pb) 및 알루미늄 하우징에 남은 우주선의 에너지를 평가하여 데드 머터리얼 Veto 컷(Cut)의 물리적 기준을 도출합니다.

### 6.2. 가상 DAQ 2x2 동시계수 분석 (Time-Stitched Coincidence)

```bash
python3 analysis_coincidence.py
```

  * Co-60 런과 우주선 런의 데이터를 읽어 들여 푸아송 분포 기반의 절대 시간(Global Time)을 부여, 타임라인을 병합(Stitching)합니다.
  * 100ns의 윈도우 내에서 우연 동시계수(Accidentals)를 걷어내고, 1.17 MeV와 1.33 MeV가 더해져 생성된 극적인 **2.50 MeV Sum Peak**를 2D 상관관계 플롯으로 명확하게 증명해냅니다.

<!-- end list -->

```
