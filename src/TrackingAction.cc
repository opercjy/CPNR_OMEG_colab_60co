#include "TrackingAction.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4TrackingManager.hh"

// static 멤버 변수 초기화
G4bool TrackingAction::s_storeOpticalTraj = false; 

void TrackingAction::PreUserTrackingAction(const G4Track* track) {
    // 1. 현재 추적 중인 입자가 '광학 광자(Optical Photon)'인지 확인합니다.
    // (감마선, 전자 등 다른 입자의 궤적은 방해하지 않습니다)
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
        
        // 2. 사용자가 매크로에서 입력한 불리언(true/false) 값에 따라 
        // 광자의 궤적(Trajectory)을 RAM에 할당할지, 생성 즉시 폐기할지 결정합니다.
        // 이를 통해 100만 런에서도 메모리가 터지지 않습니다.
        fpTrackingManager->SetStoreTrajectory(s_storeOpticalTraj);
    }
}
