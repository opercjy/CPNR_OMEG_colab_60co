import uproot
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import mplhep as hep

# CERN 논문 표준 스타일 적용 (선택 사항, 없으면 기본 스타일 적용됨)
try:
    hep.style.use(hep.style.ROOT)
except:
    pass

def load_and_assign_time(filename, rate_hz):
    """ ROOT 데이터를 로드하고 푸아송 분포에 기반한 절대 타임스탬프를 부여합니다. """
    tree = uproot.open(f"{filename}:DAQ")
    df = tree.arrays(["E0", "E1", "Truth0", "Truth1"], library="pd")
    
    # 지수 분포를 따르는 이벤트 간 시간 간격(delta T) 생성
    dt = np.random.exponential(1.0 / rate_hz, len(df))
    df['AbsTime'] = np.cumsum(dt) # 절대 우주 시간(초)
    return df

def main():
    print(">>> Virtual DAQ Time-Stitching Pipeline 시작...")

    # 1. 물리적 발생률 설정 (★ 이 수치를 바꾸면서 시뮬레이션 없이 무한 테스트 가능!)
    RATE_CO60 = 5000.0  # Co-60 방사능 (5 kBq)
    RATE_COSMIC = 100.0 # 검출기 면적에 비례한 우주선 플럭스 (100 Hz)
    COIN_WINDOW = 100e-9 # 동시계수 윈도우 (100 ns)

    # 2. 데이터 로드 및 타임스탬프 부여
    try:
        df_co60 = load_and_assign_time("Co60_Only.root", RATE_CO60)
        df_cosmic = load_and_assign_time("Cosmic_Only.root", RATE_COSMIC)
        print(f"[OK] 데이터 로드 완료: Co60({len(df_co60)}건), Cosmic({len(df_cosmic)}건)")
    except Exception as e:
        print(f"[오류] 파일을 찾을 수 없거나 열 수 없습니다: {e}")
        return

    # 3. 타임라인 병합 및 절대 시간순 정렬 (Time-Stitching)
    df = pd.concat([df_co60, df_cosmic]).sort_values(by='AbsTime').reset_index(drop=True)

    singles = []
    true_coin = []
    accidental_coin = []

    # 4. DAQ Sliding Window Logic
    print(">>> Running DAQ Coincidence Emulator...")
    i = 0
    n_events = len(df)
    
    AbsTime = df['AbsTime'].values
    E0 = df['E0'].values
    E1 = df['E1'].values
    Truth0 = df['Truth0'].values
    Truth1 = df['Truth1'].values

    while i < n_events:
        # [조건 1] 다음 이벤트와의 시간차가 CoinWindow 이내인가? (우연한 겹침 발생)
        if i < n_events - 1 and (AbsTime[i+1] - AbsTime[i]) <= COIN_WINDOW:
            sum_E0 = E0[i] + E0[i+1]
            sum_E1 = E1[i] + E1[i+1]
            
            # 두 모듈 모두에 에너지가 들어왔는가?
            if sum_E0 > 0 and sum_E1 > 0:
                # 기원이 다르면 100% 우연 동시계수 (Co60 + Cosmic 믹스)
                if Truth0[i] != Truth0[i+1]:
                    accidental_coin.append(sum_E0 + sum_E1)
                else:
                    # 같은 기원끼리 겹친 경우 (예: Co60 2번이 100ns 안에 우연히 연달아 터짐)
                    accidental_coin.append(sum_E0 + sum_E1)
            i += 2 # 두 이벤트를 묶어서 처리했으므로 한 칸 더 건너뜀
            
        else:
            # [조건 2] 겹치지 않은 단일 독립 이벤트
            if E0[i] > 0 and E1[i] > 0:
                # Geant4 내부에서 물리적으로 제대로 발생한 '진짜 동시계수' (Co-60 Cascade)
                true_coin.append(E0[i] + E1[i])
            elif E0[i] > 0 or E1[i] > 0:
                # 한쪽 모듈만 맞은 경우 (단일 히트)
                val = E0[i] if E0[i] > 0 else E1[i]
                singles.append(val)
            i += 1

    print(f"\n[분석 결과]")
    print(f"- 단일 히트(Singles): {len(singles)}건")
    print(f"- 진짜 동시계수(True Coincidence): {len(true_coin)}건")
    print(f"- 우연 동시계수(Accidental Coincidence): {len(accidental_coin)}건")

    # =========================================================================
    # [논문 출판용 스펙트럼 시각화]
    # =========================================================================
    # 광전자(PE) 개수 스케일에 맞춰 x축 범위를 0~5000 PE 정도로 설정합니다.
    fig, ax = plt.subplots(figsize=(10, 7))
    bins = np.linspace(0, 5000, 200) 

    # 데이터가 있을 때만 그리기
    if len(singles) > 0:
        hep.histplot(singles, bins=bins, ax=ax, color='black', histtype='step', 
                     linewidth=1.5, label='Singles Spectrum')
    if len(true_coin) > 0:             
        hep.histplot(true_coin, bins=bins, ax=ax, color='blue', histtype='step', 
                     linewidth=2.0, label='True Coincidence (Co-60 Cascade)')
    if len(accidental_coin) > 0:             
        hep.histplot(accidental_coin, bins=bins, ax=ax, color='red', histtype='fill', 
                     alpha=0.4, label=f'Accidental Coincidence (Window={COIN_WINDOW*1e9:.0f}ns)')

    ax.set_yscale('log')
    ax.set_xlim(0, 5000)
    ax.set_xlabel("Energy Deposit [Number of Photoelectrons (PE)]", fontsize=16)
    ax.set_ylabel("Counts / Bin", fontsize=16)
    ax.set_title("Time-Stitched Coincidence Analysis (Toy MC DAQ)", fontsize=18, pad=15)
    ax.legend(loc='upper right', fontsize=14)
    ax.grid(True, which='both', linestyle='--', alpha=0.5)

    plt.tight_layout()
    plt.savefig("Coincidence_Analysis_ToyMC.png", dpi=300)
    print("\n[SUCCESS] 그래프가 'Coincidence_Analysis_ToyMC.png'로 저장되었습니다!")
    plt.show()

if __name__ == "__main__":
    main()
