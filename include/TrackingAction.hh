#pragma once
#include "G4UserTrackingAction.hh"
#include "globals.hh"

class TrackingAction : public G4UserTrackingAction {
public:
    TrackingAction() = default;
    ~TrackingAction() override = default;

    // 입자의 궤적이 생성되기 직전에 호출되는 함수
    void PreUserTrackingAction(const G4Track* track) override;

    // [MT 동기화] 워커 스레드들이 각각 생성되더라도, 매크로에서 설정한 
    // 궤적 저장 여부(true/false)를 똑같이 공유할 수 있도록 static으로 선언합니다.
    static void SetStoreOpticalTraj(G4bool val) { s_storeOpticalTraj = val; }
    static G4bool GetStoreOpticalTraj() { return s_storeOpticalTraj; }

private:
    // 메모리 오버플로우(OOM)를 막기 위해 기본값은 항상 false(저장 안 함)입니다.
    static G4bool s_storeOpticalTraj; 
};
