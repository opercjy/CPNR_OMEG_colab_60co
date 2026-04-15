#pragma once
#include "G4VSensitiveDetector.hh"
#include "PMTHit.hh" // 🚨 독립된 PMTHit.hh를 가져오기만 합니다!

class G4Step;
class G4HCofThisEvent;

class PMTSD : public G4VSensitiveDetector {
public:
    // 🚨 ID 하드코딩 제거 (Touchable History가 알아서 해결함)
    PMTSD(const G4String& name);
    ~PMTSD() override = default;

    void Initialize(G4HCofThisEvent* hce) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;

private:
    PMTHitsCollection* fHitsCollection;
};
