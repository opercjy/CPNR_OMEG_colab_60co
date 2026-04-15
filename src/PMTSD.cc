#include "PMTSD.hh"
#include "RunAction.hh" 
#include "G4SDManager.hh"
#include "G4OpticalPhoton.hh"
#include "G4VPhysicalVolume.hh"

PMTSD::PMTSD(const G4String& name) 
    : G4VSensitiveDetector(name), fHitsCollection(nullptr) {
    collectionName.insert("HitsCollection");
}

void PMTSD::Initialize(G4HCofThisEvent* hce) {
    static G4ThreadLocal G4int hcID = -1;
    if (hcID < 0) {
        hcID = G4SDManager::GetSDMpointer()->GetCollectionID(SensitiveDetectorName + "/" + collectionName[0]);
    }
    fHitsCollection = new PMTHitsCollection(SensitiveDetectorName, collectionName[0]);
    hce->AddHitsCollection(hcID, fHitsCollection);

    fHitsCollection->insert(new PMTHit());
    fHitsCollection->insert(new PMTHit());
    (*fHitsCollection)[0]->SetModuleID(0);
    (*fHitsCollection)[1]->SetModuleID(1);
}

G4bool PMTSD::ProcessHits(G4Step* step, G4TouchableHistory*) {
    auto track = step->GetTrack();
    
    if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) return false;

    // ★ BoundaryInvokeSD(true)에 의해 25% QE 주사위를 통과한 입자들만 
    // 이곳으로 넘어오게 됩니다. 복잡한 난수나 Boundary 체크가 더 이상 필요 없습니다!

    G4int modID = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(1); 
    if (modID < 0 || modID > 1) {
        track->SetTrackStatus(fStopAndKill);
        return false;
    }
    
    // PE 카운트 즉시 +1
    PMTHit* hit = (*fHitsCollection)[modID];
    hit->UpdateTime(step->GetPostStepPoint()->GetGlobalTime());
    hit->AddPE();
    hit->SetSourceType(RunAction::GetSourceType());
    
    // 기록된 광자는 즉시 소멸
    track->SetTrackStatus(fStopAndKill); 
    return true;
}
