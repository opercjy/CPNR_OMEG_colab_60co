#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4Step.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"

SteppingAction::SteppingAction(EventAction* eventAction)
 : G4UserSteppingAction(), fEventAction(eventAction) {}

void SteppingAction::UserSteppingAction(const G4Step* step) {
    // 1. 이번 스텝에서 입자가 잃어버린 에너지(침적 에너지)를 가져옵니다.
    G4double edep = step->GetTotalEnergyDeposit();
    
    // ★ [초고속 최적화] 시뮬레이션의 99%는 에너지를 잃지 않고 튕겨다니는 
    // 광학 광자의 이동 스텝입니다. 에너지가 0이면 무거운 문자열(String) 연산을 
    // 수행하지 않고 즉시 함수를 빠져나가 CPU 캐시 미스(Cache Miss)를 방어합니다.
    if (edep <= 0.0) return;

    auto touchable = step->GetPreStepPoint()->GetTouchableHandle();
    
    // ★ [Segfault 방어] 최상위 공간(World)이나 빈 공기층을 타격했을 때는 
    // 상위 계층(Depth 1, 즉 검출기 모듈)이 존재하지 않습니다. 
    // 무작정 CopyNumber를 부르면 메모리 에러가 터지므로 이를 쳐냅니다.
    if (touchable->GetHistoryDepth() < 1) return;

    // 2. 현재 타격받은 검출기 모듈 번호(0번 고정축, 1번 이동축)를 가져옵니다.
    G4int modID = touchable->GetCopyNumber(1);
    if (modID < 0 || modID > 1) return;

    // 3. [최적화] 긴 물리적 볼륨 이름(Physical Volume) 대신, 
    // 짧은 물질(Material) 이름으로 초고속 라우팅을 수행합니다.
    const G4String& matName = step->GetPreStepPoint()->GetMaterial()->GetName();

    // 4. 물질의 종류에 따라 EventAction의 각각 다른 방(Array)에 에너지를 더해줍니다.
    if      (matName == "LS_Material")      fEventAction->AddEdep(modID, 0, edep); // 0: 섬광체
    else if (matName == "G4_Pb")            fEventAction->AddEdep(modID, 1, edep); // 1: 납 차폐체
    else if (matName == "G4_Al")            fEventAction->AddEdep(modID, 2, edep); // 2: 알루미늄
    else if (matName == "G4_POLYCARBONATE") fEventAction->AddEdep(modID, 3, edep); // 3: 3D 지지대
}
