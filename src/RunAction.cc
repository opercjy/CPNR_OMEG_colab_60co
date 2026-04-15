#include "RunAction.hh"
#include "TrackingAction.hh" 
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4GenericMessenger.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"

G4int    RunAction::s_Threshold    = 0;
G4double RunAction::s_CoinWindow   = 100.0 * ns;  
G4double RunAction::s_PileUpWindow = 50.0 * ns;   
G4double RunAction::s_CurrentAngle = 0.0 * deg; 
G4int    RunAction::s_SourceType   = 1; 

RunAction::RunAction() : G4UserRunAction(), fDaqMessenger(nullptr), fTrkMessenger(nullptr) {
    // ★ [마스터 스레드 방어] 
    // UI Command는 시뮬레이션을 통제하는 마스터(Master)에서만 등록되어야 합니다.
    // Worker 스레드 16개가 이걸 동시에 등록하려 하면 터지거나 명령어가 무시됩니다.
    if (G4Threading::IsMasterThread() || !G4Threading::IsMultithreadedApplication()) {
        
        fDaqMessenger = new G4GenericMessenger(this, "/myApp/daq/", "DAQ Emulator Controls");
        fDaqMessenger->DeclareMethod("setThreshold", &RunAction::SetThreshold, "Trigger threshold in PE (No unit)");
        fDaqMessenger->DeclareMethodWithUnit("setCoinWindow", "ns", &RunAction::SetCoinWindow, "Coincidence resolving time");
        fDaqMessenger->DeclareMethodWithUnit("setPileUpWindow", "ns", &RunAction::SetPileUpWindow, "Pile-up rejection window");
        fDaqMessenger->DeclareMethodWithUnit("setCurrentAngle", "deg", &RunAction::SetCurrentAngle, "Detector angle");
        fDaqMessenger->DeclareMethod("setSourceType", &RunAction::SetSourceType, "MC Truth (0=CRY, 1=Co60)");

        // OOM 방어 Tracking 제어 UI 커맨드 우회 등록 (Wrapper 호출)
        fTrkMessenger = new G4GenericMessenger(this, "/myApp/tracking/", "Tracking Controls");
        fTrkMessenger->DeclareMethod("storeOpticalTraj", &RunAction::SetStoreOpticalTrajWrapper, "Store optical photon trajectories (true/false)");
    }

    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetFileName("Default_Output"); 
    
    // ★ 16개의 Worker가 만든 임시 파일들을 런 종료 시 1개의 파일로 묶어줍니다!
    analysisManager->SetNtupleMerging(true);

    analysisManager->CreateNtuple("DAQ", "Coincidence Data");
    analysisManager->CreateNtupleDColumn("E0");        // 0
    analysisManager->CreateNtupleDColumn("E1");        // 1
    analysisManager->CreateNtupleDColumn("t0");        // 2
    analysisManager->CreateNtupleDColumn("t1");        // 3
    analysisManager->CreateNtupleIColumn("isCoin");    // 4
    analysisManager->CreateNtupleIColumn("isPileUp0"); // 5
    analysisManager->CreateNtupleIColumn("isPileUp1"); // 6
    analysisManager->CreateNtupleDColumn("Angle");     // 7
    analysisManager->CreateNtupleIColumn("Truth0");    // 8
    analysisManager->CreateNtupleIColumn("Truth1");    // 9

    // ★ 10번 ~ 17번: 모듈별/부품별 순수 에너지 스코어링 컬럼 개통
    analysisManager->CreateNtupleDColumn("EdepLS0");   // 10
    analysisManager->CreateNtupleDColumn("EdepLS1");   // 11
    analysisManager->CreateNtupleDColumn("EdepPb0");   // 12
    analysisManager->CreateNtupleDColumn("EdepPb1");   // 13
    analysisManager->CreateNtupleDColumn("EdepAl0");   // 14
    analysisManager->CreateNtupleDColumn("EdepAl1");   // 15
    analysisManager->CreateNtupleDColumn("EdepPETG0"); // 16
    analysisManager->CreateNtupleDColumn("EdepPETG1"); // 17

    analysisManager->FinishNtuple();
}

RunAction::~RunAction() {
    // 동적 할당된 메신저들을 지워주어 프로그램 종료 시 메모리 누수를 막습니다.
    if (fDaqMessenger) delete fDaqMessenger;
    if (fTrkMessenger) delete fTrkMessenger; 
}

// 래퍼 함수(Wrapper)를 통한 TrackingAction 제어 분기
void RunAction::SetStoreOpticalTrajWrapper(G4bool val) {
    TrackingAction::SetStoreOpticalTraj(val);
}

void RunAction::BeginOfRunAction(const G4Run*) {
    G4AnalysisManager::Instance()->OpenFile(); 
}

void RunAction::EndOfRunAction(const G4Run*) {
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();
}
