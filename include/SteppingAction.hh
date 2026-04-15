#pragma once
#include "G4UserSteppingAction.hh"
#include "globals.hh"

class EventAction; // EventAction 클래스 전방 선언 (의존성 주입용)

class SteppingAction : public G4UserSteppingAction {
public:
    // 생성자에서 EventAction의 포인터를 받아옵니다. (데이터를 넘겨주기 위함)
    SteppingAction(EventAction* eventAction);
    ~SteppingAction() override = default;

    // 입자가 한 스텝(Step) 이동할 때마다 호출되는 핵심 함수
    void UserSteppingAction(const G4Step* step) override;

private:
    EventAction* fEventAction; // 에너지를 누적할 EventAction의 주소값
};
