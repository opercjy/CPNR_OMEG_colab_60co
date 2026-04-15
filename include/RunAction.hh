#pragma once
#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;
class G4GenericMessenger;

class RunAction : public G4UserRunAction {
public:
    RunAction();
    ~RunAction() override;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

    // DAQ 제어용 Setter
    void SetThreshold(G4int val)      { s_Threshold = val; }
    void SetCoinWindow(G4double val)  { s_CoinWindow = val; }
    void SetPileUpWindow(G4double val){ s_PileUpWindow = val; }
    void SetCurrentAngle(G4double val){ s_CurrentAngle = val; }
    void SetSourceType(G4int val)     { s_SourceType = val; }

    // ★ [핵심: MT 메신저 방어] TrackingAction 제어를 위한 Delegate(Wrapper) 함수
    // G4GenericMessenger는 static 함수를 엮을 때 포인터 에러가 발생하기 쉽습니다.
    // 이를 막기 위해 RunAction의 멤버 함수인 이 함수가 중간에서 다리 역할을 합니다.
    void SetStoreOpticalTrajWrapper(G4bool val); 

    // Worker 스레드들이 접근할 Getter
    static G4int    GetThreshold()    { return s_Threshold; }
    static G4double GetCoinWindow()   { return s_CoinWindow; }
    static G4double GetPileUpWindow() { return s_PileUpWindow; }
    static G4double GetCurrentAngle() { return s_CurrentAngle; }
    static G4int    GetSourceType()   { return s_SourceType; }

private:
    static G4int    s_Threshold; 
    static G4double s_CoinWindow;
    static G4double s_PileUpWindow;
    static G4double s_CurrentAngle;
    static G4int    s_SourceType;

    // 메신저 포인터들
    G4GenericMessenger* fDaqMessenger;
    G4GenericMessenger* fTrkMessenger;
};
