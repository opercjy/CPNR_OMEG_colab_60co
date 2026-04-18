#pragma once
#include "G4VUserDetectorConstruction.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4GenericMessenger;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // 매크로 제어용 함수
    void SetMovablePMTAngle(G4double angle);
    void SetFrontDistance(G4double dist);

private:
    void UpdateDetectorPosition();
    G4LogicalVolume* ConstructDetectorUnit();

    G4GenericMessenger* fMessenger;
    
    G4double fMovablePMTAngle;
    G4double fFrontDistance; // ★ 선원 ~ 전면부 표면 거리

    G4LogicalVolume* fLogicPhotocathode;
    
    G4VPhysicalVolume* fFixedPhys;
    G4VPhysicalVolume* fMovablePhys;
    G4RotationMatrix* fMovableRot;
};
