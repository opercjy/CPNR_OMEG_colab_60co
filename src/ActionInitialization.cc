#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "TrackingAction.hh"
#include "SteppingAction.hh"

void ActionInitialization::BuildForMaster() const {
    // 마스터 스레드는 파일 쓰기와 UI 명령어 제어만 담당합니다.
    SetUserAction(new RunAction());
}

void ActionInitialization::Build() const {
    // 워커 스레드는 실제로 입자를 쏘고, 데이터를 분석합니다.
    SetUserAction(new PrimaryGeneratorAction());
    SetUserAction(new RunAction());
    
    // ★ 의존성 주입(Dependency Injection): 
    // EventAction 객체를 먼저 만든 뒤, 그 '주소값(포인터)'을 
    // SteppingAction 생성자에 넘겨주어, SteppingAction이 수집한 에너지를 
    // EventAction의 Array 방에 직접 꽂아 넣을 수 있게 이어줍니다.
    auto eventAction = new EventAction();
    SetUserAction(eventAction);
    SetUserAction(new SteppingAction(eventAction)); 
    
    // 궤적 메모리 암살자 등록
    SetUserAction(new TrackingAction());
}
