#pragma once
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleDefinition.hh"
#include "globals.hh"

class G4Event;
class CRYGenerator;
class CRYSetup;
class G4GeneralParticleSource; // ★ GPS 포인터 추가

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;
    
    void GeneratePrimaries(G4Event* event) override;

private:
    G4ParticleDefinition* GetParticleDef(int cryId);
    
    CRYGenerator* fCRYGenerator;
    CRYSetup* fCRYSetup;
    
    G4GeneralParticleSource* fGPS; // ★ GPS 변수 추가
};
