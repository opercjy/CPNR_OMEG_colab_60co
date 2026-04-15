#pragma once

#include "G4VUserActionInitialization.hh"

class ActionInitialization : public G4VUserActionInitialization {
public:
    ActionInitialization() = default;
    ~ActionInitialization() override = default;

    // 마스터 스레드용 (RunAction 등 전역 관리용)
    void BuildForMaster() const override;

    // 워커 스레드용 (각 스레드별로 독립적인 Action 객체 생성)
    void Build() const override;
};
