#include "MyPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4NuclearLevelData.hh"
#include "G4DeexPrecoParameters.hh"

MyPhysicsList::MyPhysicsList() : G4VModularPhysicsList() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4RadioactiveDecayPhysics());
    
    RegisterPhysics(new G4OpticalPhysics());
}

void MyPhysicsList::ConstructProcess() {
    G4VModularPhysicsList::ConstructProcess();

    G4DeexPrecoParameters* deex = G4NuclearLevelData::GetInstance()->GetParameters();
    deex->SetCorrelatedGamma(true);

    auto optParams = G4OpticalParameters::Instance();
    optParams->SetScintTrackSecondariesFirst(true);
    optParams->SetCerenkovTrackSecondariesFirst(true);

    // ★ [연구원님 기법 이식] PMT 경계면에서 엔진이 QE 주사위를 굴린 뒤 SD를 호출하게 강제함!
    optParams->SetBoundaryInvokeSD(true);
}
