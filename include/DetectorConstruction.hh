#pragma once
#include "G4VUserDetectorConstruction.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4GenericMessenger;
class G4Tubs;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // --- Dynamic Geometry Control ---
    void SetMovablePMTAngle(G4double angle);
    void SetDetectorDistance(G4double dist);
    void SetDetectorBaseRadius(G4double radius);

private:
    void UpdateMovableDetectorPosition();
    void UpdateDetectorSize();
    G4LogicalVolume* ConstructDetectorUnit();

    G4GenericMessenger* fMessenger;
    
    G4double fMovablePMTAngle;
    G4double fDetectorDistance;
    G4double fBaseRadius;

    G4LogicalVolume* fLogicPhotocathode;
    
    // 배치 포인터 (대칭 이동용)
    G4VPhysicalVolume* fFixedPhys;
    G4VPhysicalVolume* fMovablePhys;
    G4RotationMatrix* fMovableRot;

    // 솔리드 포인터 (동적 크기 연쇄 조절용)
    G4Tubs* fSolidLS;
    G4Tubs* fSolidDuranBack;
    G4Tubs* fSolidGrease;
    G4Tubs* fSolidWin;
    G4Tubs* fSolidCat;
    
    G4Tubs* fSolidDuranSide;
    G4Tubs* fSolidDuranFront;
    
    G4Tubs* fSolidTyvekSide;
    G4Tubs* fSolidTyvekFront;
    
    G4Tubs* fSolidAlSide;
    G4Tubs* fSolidAlFront;
    
    G4Tubs* fSolidPetgSide;
    G4Tubs* fSolidPetgFront;
    G4Tubs* fSolidPetgBack;
    
    G4Tubs* fSolidPbSide;
};
