#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh" // ★ EventAction.hh 대신 RunAction.hh를 포함합니다!
#include "G4Event.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "G4GeneralParticleSource.hh" 

#include "G4Gamma.hh"
#include "G4Electron.hh"
#include "G4Positron.hh"
#include "G4MuonMinus.hh"
#include "G4MuonPlus.hh"
#include "G4PionMinus.hh"
#include "G4PionPlus.hh"
#include "G4KaonMinus.hh"
#include "G4KaonPlus.hh"
#include "G4Proton.hh"
#include "G4Neutron.hh"

#include "CRYSetup.h"
#include "CRYGenerator.h"
#include "CRYParticle.h"
#include "CRYUtils.h"

#include <cstdlib>
#include <cmath>
#include "Randomize.hh" 

// CRY MT 난수 안전 보장
double CRY_RNG() {
    return G4UniformRand();
}

PrimaryGeneratorAction::PrimaryGeneratorAction() 
 : G4VUserPrimaryGeneratorAction(), fCRYGenerator(nullptr), fCRYSetup(nullptr), fGPS(nullptr) 
{
    // 1. GPS 초기화 (Co-60)
    fGPS = new G4GeneralParticleSource();

    // 2. CRY 초기화 (Cosmic Ray)
    const char* dataPath = std::getenv("CRYDATAPATH");
    if (!dataPath) {
        G4Exception("PrimaryGeneratorAction", "CRY_001", FatalException,
                    "CRYDATAPATH is not set! Please check your environment variables.");
    }

    G4String setupString = "returnNeutrons 1 returnProtons 1 returnGammas 1 "
                           "returnElectrons 1 returnMuons 1 returnPions 1 "
                           "date 7-1-2026 latitude 35.1 altitude 0 subboxLength 1";

    fCRYSetup = new CRYSetup(setupString, dataPath);
    fCRYSetup->setRandomFunction(CRY_RNG);
    fCRYGenerator = new CRYGenerator(fCRYSetup);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete fCRYGenerator;
    delete fCRYSetup;
    delete fGPS; 
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
    
    // ★ 수정됨: RunAction에서 SourceType을 가져옵니다!
    G4int currentSourceType = RunAction::GetSourceType();

    // =========================================================================
    // ★ 분리 생성 아키텍처 (Decoupled Generation)
    // 0: 우주선만 생성, 1: Co-60만 생성. 절대 한 이벤트에 섞지 않습니다!
    // =========================================================================

    if (currentSourceType == 1) {
        // [시그널 런] Co-60 붕괴
        fGPS->GeneratePrimaryVertex(event);
    } 
    else if (currentSourceType == 0) {
        // [백그라운드 런] 우주선
        std::vector<CRYParticle*> cryParticles;
        fCRYGenerator->genEvent(&cryParticles);

        for (auto p : cryParticles) {
            G4ParticleDefinition* partDef = GetParticleDef(p->id());
            if (!partDef) {
                delete p; 
                continue;
            }

            auto particle = new G4PrimaryParticle(partDef);
            
            G4double mass = partDef->GetPDGMass();
            G4double eKin = p->ke() * MeV;
            G4double pMag = std::sqrt(eKin * eKin + 2.0 * mass * eKin);

            particle->SetMomentum(p->u() * pMag,
                                  p->w() * pMag,  
                                  p->v() * pMag); 
            
            particle->SetMass(mass);
            particle->SetCharge(partDef->GetPDGCharge());

            G4ThreeVector pos(p->x() * m, 50.0 * cm, p->y() * m);
            
            auto vertex = new G4PrimaryVertex(pos, p->t() * second);
            vertex->SetPrimary(particle);
            
            event->AddPrimaryVertex(vertex);

            delete p; 
        }
    }
}

G4ParticleDefinition* PrimaryGeneratorAction::GetParticleDef(int cryId) {
    switch (cryId) {
        case 1:  return G4Neutron::Neutron();
        case 2:  return G4Proton::Proton();
        case 3:  return G4PionMinus::PionMinus();
        case 4:  return G4PionPlus::PionPlus();
        case 5:  return G4KaonMinus::KaonMinus();
        case 6:  return G4KaonPlus::KaonPlus();
        case 7:  return G4MuonMinus::MuonMinus();
        case 8:  return G4MuonPlus::MuonPlus();
        case 9:  return G4Electron::Electron();
        case 10: return G4Positron::Positron();
        case 11: return G4Gamma::Gamma();
        default: return nullptr;
    }
}
