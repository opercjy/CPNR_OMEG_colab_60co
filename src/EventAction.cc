#include "EventAction.hh"
#include "RunAction.hh"
#include "PMTHit.hh" 
#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include <cmath>

EventAction::EventAction() : G4UserEventAction(), fHCID(-1) {
    // 생성 시 배열을 안전하게 0으로 덮어둡니다.
    for (int i = 0; i < 2; ++i) {
        fEdepLS[i] = 0.0; fEdepPb[i] = 0.0; fEdepAl[i] = 0.0; fEdepPETG[i] = 0.0;
    }
}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event*) {
    if (fHCID == -1) {
        fHCID = G4SDManager::GetSDMpointer()->GetCollectionID("PMTSD/HitsCollection"); 
    }
    // ★ [데이터 레이스 방지] 
    // 이전 이벤트에서 누적된 에너지가 다음 이벤트로 이월되지 않도록 무조건 0.0으로 초기화!
    for (int i = 0; i < 2; ++i) {
        fEdepLS[i]   = 0.0;
        fEdepPb[i]   = 0.0;
        fEdepAl[i]   = 0.0;
        fEdepPETG[i] = 0.0;
    }
}

void EventAction::EndOfEventAction(const G4Event* event) {
    // ★ [Veto 보존 로직] 
    // 우주선(Cosmic)이 검출기를 관통할 때, 빛을 내는 LS나 PMT는 빗겨가고 
    // 납(Pb)이나 알루미늄 껍데기만 때리고 지나가는 경우가 있습니다.
    // 백그라운드 분석을 위해서는 이런 이벤트도 버리지 않고 기록해 두어야 합니다!
    G4bool hasVetoEnergy = (fEdepLS[0]>0 || fEdepLS[1]>0 || fEdepPb[0]>0 || fEdepPb[1]>0 || 
                            fEdepAl[0]>0 || fEdepAl[1]>0 || fEdepPETG[0]>0 || fEdepPETG[1]>0);

    if (fHCID < 0) return;
    auto hce = event->GetHCofThisEvent();
    if (!hce) return;
    auto hitsCollection = static_cast<PMTHitsCollection*>(hce->GetHC(fHCID));
    if (!hitsCollection) return;

    PMTHit* hit0 = (*hitsCollection)[0];
    PMTHit* hit1 = (*hitsCollection)[1];

    G4int pe0 = hit0->GetPE();
    G4int pe1 = hit1->GetPE();

    // 1. 트리거(임계값) 판별
    G4int thr = RunAction::GetThreshold();
    G4bool isTrig[2] = { pe0 >= thr, pe1 >= thr };
    
    // PMT도 트리거되지 않았고, 차폐체에 맞은 에너지도 없으면 "완전한 쓰레기"이므로 버립니다.
    if (!isTrig[0] && !isTrig[1] && !hasVetoEnergy) return;

    // 2. 파일업(Pile-up) 및 동시계수(Coincidence) 판별
    G4bool isPileUp[2] = { false, false };
    G4double pWindow = RunAction::GetPileUpWindow();
    G4double cWindow = RunAction::GetCoinWindow();

    if (isTrig[0] && (hit0->GetTimeLast() - hit0->GetTimeFirst() > pWindow)) isPileUp[0] = true;
    if (isTrig[1] && (hit1->GetTimeLast() - hit1->GetTimeFirst() > pWindow)) isPileUp[1] = true;

    G4bool isCoin = false;
    if (isTrig[0] && isTrig[1]) {
        if (std::abs(hit0->GetTimeFirst() - hit1->GetTimeFirst()) <= cWindow) isCoin = true;
    }

    auto analysisManager = G4AnalysisManager::Instance();
    G4double saveT0 = isTrig[0] ? hit0->GetTimeFirst() : -1.0;
    G4double saveT1 = isTrig[1] ? hit1->GetTimeFirst() : -1.0;

    // 3. 기존 0~9번 광전자 및 타이밍 칼럼 저장
    analysisManager->FillNtupleDColumn(0, static_cast<G4double>(pe0));
    analysisManager->FillNtupleDColumn(1, static_cast<G4double>(pe1));
    analysisManager->FillNtupleDColumn(2, saveT0);
    analysisManager->FillNtupleDColumn(3, saveT1);
    analysisManager->FillNtupleIColumn(4, isCoin ? 1 : 0);
    analysisManager->FillNtupleIColumn(5, isPileUp[0] ? 1 : 0);
    analysisManager->FillNtupleIColumn(6, isPileUp[1] ? 1 : 0);
    analysisManager->FillNtupleDColumn(7, RunAction::GetCurrentAngle());
    analysisManager->FillNtupleIColumn(8, isTrig[0] ? hit0->GetSourceType() : -1);
    analysisManager->FillNtupleIColumn(9, isTrig[1] ? hit1->GetSourceType() : -1);

    // ========================================================================
    // ★ 10번~17번 컬럼: 순수 침적 에너지(True Energy Deposition) 추가 저장
    // (이 값과 0번/1번의 PE를 비교하면 PMT의 에너지 분해능 효과를 관찰할 수 있습니다)
    // ========================================================================
    analysisManager->FillNtupleDColumn(10, fEdepLS[0]);
    analysisManager->FillNtupleDColumn(11, fEdepLS[1]);
    analysisManager->FillNtupleDColumn(12, fEdepPb[0]);
    analysisManager->FillNtupleDColumn(13, fEdepPb[1]);
    analysisManager->FillNtupleDColumn(14, fEdepAl[0]);
    analysisManager->FillNtupleDColumn(15, fEdepAl[1]);
    analysisManager->FillNtupleDColumn(16, fEdepPETG[0]);
    analysisManager->FillNtupleDColumn(17, fEdepPETG[1]);

    analysisManager->AddNtupleRow(); // ROOT 트리 한 줄 쓰기 완료
}
