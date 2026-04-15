#pragma once
#include "G4UserEventAction.hh"
#include "globals.hh"

class EventAction : public G4UserEventAction {
public:
    EventAction();
    ~EventAction() override;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    // SteppingAction에서 에너지를 쏠아주는(Inject) 헬퍼 메서드
    void AddEdep(G4int modID, G4int matID, G4double edep) {
        if      (matID == 0) fEdepLS[modID]   += edep;
        else if (matID == 1) fEdepPb[modID]   += edep;
        else if (matID == 2) fEdepAl[modID]   += edep;
        else if (matID == 3) fEdepPETG[modID] += edep;
    }

private:
    G4int fHCID = -1; // PMT 히트 컬렉션 ID

    // ★ 모듈별(크기 2) 4가지 물질의 순수 침적 에너지를 담을 배열들
    G4double fEdepLS[2];
    G4double fEdepPb[2];
    G4double fEdepAl[2];
    G4double fEdepPETG[2];
};
