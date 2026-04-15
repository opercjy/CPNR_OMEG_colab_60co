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

    // --- Dynamic Geometry Control ---
    void SetMovablePMTAngle(G4double angle);
    void SetDetectorDistance(G4double dist);

private:
    void UpdateMovableDetectorPosition();
    G4LogicalVolume* ConstructDetectorUnit();

    G4GenericMessenger* fMessenger;
    
    G4double fMovablePMTAngle;
    G4double fDetectorDistance;

    G4LogicalVolume* fLogicPhotocathode;
    G4VPhysicalVolume* fMovablePhys;
    G4RotationMatrix* fMovableRot;
};
