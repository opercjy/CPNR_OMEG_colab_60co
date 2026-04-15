import uproot
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import mplhep as hep
import os
import warnings

# 불필요한 경고 무시
warnings.filterwarnings("ignore", category=UserWarning)
warnings.filterwarnings("ignore", category=RuntimeWarning)

# 고에너지 물리(HEP) 논문용 표준 스타일 적용
hep.style.use(hep.style.ROOT)

# =========================================================================
# [각도 스캔 처리용 플레이스홀더 - 주석 처리됨]
# 향후 여러 각도를 터미널에서 한 번에 입력받아 처리할 때 아래 주석을 해제하세요.
# 실행 예시: python3 plot_resolution.py --angles 30,45,90,180
# =========================================================================
# import argparse
# def parse_arguments():
#     parser = argparse.ArgumentParser(description="Co-60 Coincidence Angular Scan Analysis")
#     parser.add_argument("--angles", type=str, default="180", help="Comma separated angles (e.g., 30,60,90,180)")
#     args = parser.parse_args()
#     return [int(a) for a in args.angles.split(",")]

def analyze_and_plot(files_info, CALIBRATION_FACTOR, angle_tag=""):
    """ 단일 각도에 대한 데이터 로드 및 2개의 플롯(Resolution, Coincidence) 생성 """
    
    print(f"\n[{angle_tag}] 데이터 로드 및 분석을 시작합니다...")
    data = {}

    # 1. 데이터 일괄 추출
    for label, info in files_info.items():
        fname = info["file"]
        if not os.path.exists(fname):
            print(f"❌ 파일을 찾을 수 없습니다: {fname}")
            continue

        try:
            with uproot.open(f"{fname}:DAQ") as tree:
                # [분해능 및 데드 머터리얼용]
                edep_ls   = tree["EdepLS0"].array(library="np")
                edep_pb   = tree["EdepPb0"].array(library="np")
                edep_al   = tree["EdepAl0"].array(library="np")
                edep_petg = tree["EdepPETG0"].array(library="np")
                
                # [코인시던스 분석용] - PE를 즉시 MeV 단위로 변환
                e0 = tree["E0"].array(library="np") / CALIBRATION_FACTOR
                e1 = tree["E1"].array(library="np") / CALIBRATION_FACTOR
                t0 = tree["t0"].array(library="np")
                t1 = tree["t1"].array(library="np")
                is_coin = tree["isCoin"].array(library="np")

                data[label] = {
                    "edep_ls": edep_ls, "edep_pb": edep_pb, "edep_al": edep_al, "edep_petg": edep_petg,
                    "e0": e0, "e1": e1, "t0": t0, "t1": t1, "is_coin": is_coin,
                    "info": info
                }
        except Exception as e:
            print(f"❌ 데이터 로드 에러 ({fname}): {e}")

    if not data:
        print("분석할 데이터가 없습니다.")
        return

    # =========================================================================
    # [Figure 1] 분해능(Resolution) 및 데드 머터리얼(Dead Material) 캔버스
    # =========================================================================
    fig1, axes1 = plt.subplots(2, 2, figsize=(18, 14))
    
    for idx, (label, d) in enumerate(data.items()):
        # 유효 이벤트 필터링
        edep_ls_clean = d["edep_ls"][d["edep_ls"] > 0]
        reco_energy = d["e0"][d["e0"] > 0]

        # --- Panel 1 (Left): True Energy vs Reconstructed Energy ---
        ax_res = axes1[idx, 0]
        bins_energy = np.linspace(0, 3.5, 200)

        # 면적 정규화 및 히스토그램 사전 계산
        if len(edep_ls_clean) > 0:
            weights_ls = np.ones_like(edep_ls_clean) / len(edep_ls_clean)
            counts_ls, _ = np.histogram(edep_ls_clean, bins=bins_energy, weights=weights_ls)
            hep.histplot(counts_ls, bins=bins_energy, ax=ax_res, color='black', histtype='step', 
                         linewidth=2.0, label='True Edep in LS')
            
        if len(reco_energy) > 0:
            weights_reco = np.ones_like(reco_energy) / len(reco_energy)
            counts_reco, _ = np.histogram(reco_energy, bins=bins_energy, weights=weights_reco)
            hep.histplot(counts_reco, bins=bins_energy, ax=ax_res, color=d["info"]["color"], histtype='step', 
                         linewidth=2.0, alpha=0.8, label='Reconstructed Energy (from PE)')

        ax_res.set_yscale('log')
        ax_res.set_xlim(0, 3.5)
        ax_res.set_ylim(1e-5, 1.0)
        ax_res.set_xlabel("Energy (MeV)", fontsize=18, loc='right')
        ax_res.set_ylabel("Normalized Probability (A.U.)", fontsize=18, loc='top')
        title_tag = f" ({angle_tag})" if angle_tag != "Single" else ""
        ax_res.set_title(f"{label}{title_tag}: Detector Resolution Convolution", fontsize=22, pad=15)
        ax_res.legend(loc='upper right', fontsize=14)
        ax_res.grid(True, which="both", linestyle="--", alpha=0.5)

        # --- Panel 2 (Right): Dead Material Interactions ---
        ax_dead = axes1[idx, 1]
        bins_dead = np.linspace(0.01, 5.0, 200)

        edep_pb_clean = d["edep_pb"][d["edep_pb"] > 0]
        edep_al_clean = d["edep_al"][d["edep_al"] > 0]
        edep_petg_clean = d["edep_petg"][d["edep_petg"] > 0]

        counts_pb, _ = np.histogram(edep_pb_clean, bins=bins_dead)
        counts_al, _ = np.histogram(edep_al_clean, bins=bins_dead)
        counts_petg, _ = np.histogram(edep_petg_clean, bins=bins_dead)

        hep.histplot(counts_pb, bins=bins_dead, ax=ax_dead, color='dimgray', histtype='step', 
                     linewidth=2.0, label=f'Lead Shield (Pb) [{len(edep_pb_clean):,} hits]')
        hep.histplot(counts_al, bins=bins_dead, ax=ax_dead, color='royalblue', histtype='step', 
                     linewidth=2.0, label=f'Al Housing (Al) [{len(edep_al_clean):,} hits]')
        hep.histplot(counts_petg, bins=bins_dead, ax=ax_dead, color='darkorange', histtype='step', 
                     linewidth=2.0, label=f'3D Support (PETG) [{len(edep_petg_clean):,} hits]')

        ax_dead.set_yscale('log')
        ax_dead.set_xlim(0, 5.0)
        ax_dead.set_xlabel("Deposited Energy (MeV)", fontsize=18, loc='right')
        ax_dead.set_ylabel("Counts / Bin", fontsize=18, loc='top')
        ax_dead.set_title(f"{label}{title_tag}: Dead Material Hits", fontsize=22, pad=15)
        ax_dead.legend(loc='upper right', fontsize=14)
        ax_dead.grid(True, which="both", linestyle="--", alpha=0.5)

    fig1.tight_layout(pad=3.0)
    out_name1 = f"Fig1_Resolution_and_DeadMat_{angle_tag}.png"
    fig1.savefig(out_name1, dpi=300, bbox_inches='tight')

    # =========================================================================
    # [Figure 2] 동시계수 코인시던스(Coincidence) 2x2 심층 분석 캔버스
    # =========================================================================
    fig2, axes2 = plt.subplots(2, 2, figsize=(18, 14))

    # --- 코인시던스 데이터 필터링 ---
    coin_data = {}
    for label, d in data.items():
        mask = (d["is_coin"] == 1)
        coin_data[label] = {
            "e0": d["e0"][mask],
            "e1": d["e1"][mask],
            "dt": d["t0"][mask] - d["t1"][mask],
            "sum": d["e0"][mask] + d["e1"][mask],
            "info": d["info"]
        }

    # --- Panel [0, 0]: Co-60 2D Energy Correlation ---
    if "Co-60 Cascade" in coin_data:
        ax = axes2[0, 0]
        c = coin_data["Co-60 Cascade"]
        h = ax.hist2d(c["e0"], c["e1"], bins=100, range=[[0, 2], [0, 2]], 
                      cmap=c["info"]["cmap"], norm=LogNorm())
        fig2.colorbar(h[3], ax=ax, label="Counts / Bin")
        ax.set_title(f"Co-60: $E_0$ vs $E_1$ Correlation{title_tag}", fontsize=22, pad=15)
        ax.set_xlabel("Module 0 Energy (MeV)", fontsize=18)
        ax.set_ylabel("Module 1 Energy (MeV)", fontsize=18)
        ax.grid(True, linestyle="--", alpha=0.3)

    # --- Panel [0, 1]: Cosmic Ray 2D Energy Correlation ---
    if "Cosmic Ray" in coin_data:
        ax = axes2[0, 1]
        c = coin_data["Cosmic Ray"]
        h = ax.hist2d(c["e0"], c["e1"], bins=100, range=[[0, 10], [0, 10]], 
                      cmap=c["info"]["cmap"], norm=LogNorm())
        fig2.colorbar(h[3], ax=ax, label="Counts / Bin")
        ax.set_title(f"Cosmic Ray: $E_0$ vs $E_1$ Correlation{title_tag}", fontsize=22, pad=15)
        ax.set_xlabel("Module 0 Energy (MeV)", fontsize=18)
        ax.set_ylabel("Module 1 Energy (MeV)", fontsize=18)
        ax.grid(True, linestyle="--", alpha=0.3)

    # --- Panel [1, 0]: Timing Difference (Delta t) ---
    ax_time = axes2[1, 0]
    bins_dt = np.linspace(-20, 20, 150)
    for label, c in coin_data.items():
        if len(c["dt"]) > 0:
            counts_dt, _ = np.histogram(c["dt"], bins=bins_dt)
            hep.histplot(counts_dt, bins=bins_dt, ax=ax_time, label=label, 
                         color=c["info"]["color"], histtype='step', linewidth=2.5)
            
    ax_time.set_yscale('log')
    ax_time.set_ylim(bottom=0.5)
    ax_time.set_title(rf"Coincidence Timing Resolution ($\Delta t = t_0 - t_1$)", fontsize=22, pad=15)
    ax_time.set_xlabel(r"Time Difference $\Delta t$ (ns)", fontsize=18)
    ax_time.set_ylabel("Counts", fontsize=18)
    ax_time.legend(loc='upper right', fontsize=16)
    ax_time.grid(True, which="both", linestyle="--", alpha=0.5)

    # --- Panel [1, 1]: Sum Energy Spectrum (E0 + E1) ---
    ax_sum = axes2[1, 1]
    bins_sum = np.linspace(0, 5, 250)
    for label, c in coin_data.items():
        if len(c["sum"]) > 0:
            counts_sum, _ = np.histogram(c["sum"], bins=bins_sum)
            hep.histplot(counts_sum, bins=bins_sum, ax=ax_sum, label=label, 
                         color=c["info"]["color"], histtype='step', linewidth=2.5)
            
    if "Co-60 Cascade" in coin_data and len(coin_data["Co-60 Cascade"]["sum"]) > 0:
        ax_sum.annotate('Sum Peak\n(1.17 + 1.33 = 2.50 MeV)', xy=(2.50, 1e2), xytext=(3.5, 5e2),
                        arrowprops=dict(facecolor='black', shrink=0.05, width=1.5, headwidth=8),
                        fontsize=14, fontweight='bold', ha='center')

    ax_sum.set_yscale('log')
    ax_sum.set_ylim(0.5, 1e5)
    ax_sum.set_title(f"Coincidence Sum Energy ($E_0 + E_1$)", fontsize=22, pad=15)
    ax_sum.set_xlabel("Sum Energy (MeV)", fontsize=18)
    ax_sum.set_ylabel("Counts / Bin", fontsize=18)
    ax_sum.legend(loc='upper right', fontsize=16)
    ax_sum.grid(True, which="both", linestyle="--", alpha=0.5)

    fig2.tight_layout(pad=3.0)
    out_name2 = f"Fig2_Coincidence_Analysis_{angle_tag}.png"
    fig2.savefig(out_name2, dpi=300, bbox_inches='tight')

    print(f"✨ 플롯 생성 완료:\n 1) {out_name1}\n 2) {out_name2}")


def main():
    CALIBRATION_FACTOR = 2500.0

    # [미래의 다중 각도 스캔 로직 - 주석 해제 시 사용]
    # angles = parse_arguments()
    # for angle in angles:
    #     files_info = {
    #         "Co-60 Cascade": {"file": f"Co60_Angle_{angle}.root", "color": "blue", "cmap": "Blues"},
    #         "Cosmic Ray":    {"file": "Cosmic_Only.root", "color": "red", "cmap": "Reds"} # 우주선은 공통 사용
    #     }
    #     analyze_and_plot(files_info, CALIBRATION_FACTOR, angle_tag=f"{angle}deg")
    #
    # print("\n🎉 모든 각도의 스캔 및 플로팅이 완료되었습니다!")

    # =========================================================================
    # 현재 기본 실행 로직 (단일 파일)
    # =========================================================================
    files_info = {
        "Co-60 Cascade": {"file": "Co60_Only.root", "color": "blue", "cmap": "Blues"},
        "Cosmic Ray":    {"file": "Cosmic_Only.root", "color": "red", "cmap": "Reds"}
    }
    analyze_and_plot(files_info, CALIBRATION_FACTOR, angle_tag="Single")
    
    plt.show() # 두 개의 창을 동시에 띄웁니다.

if __name__ == "__main__":
    main()
