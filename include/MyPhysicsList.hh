#pragma once
#include "G4VModularPhysicsList.hh"

class MyPhysicsList : public G4VModularPhysicsList {
public:
    MyPhysicsList();
    ~MyPhysicsList() override = default;

    void ConstructProcess() override;
};
