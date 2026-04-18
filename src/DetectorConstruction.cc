#include "DetectorConstruction.hh"
#include "PMTSD.hh"
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpticalSurface.hh"
#include "G4SDManager.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4GenericMessenger.hh"
#include <vector>

DetectorConstruction::DetectorConstruction()
 : G4VUserDetectorConstruction(), 
   fMovablePMTAngle(45.0*deg), 
   fFrontDistance(15.0*cm), // ★ 10cm에서 15cm로 기본값 변경
   fLogicPhotocathode(nullptr), 
   fFixedPhys(nullptr), fMovablePhys(nullptr), fMovableRot(nullptr)
{
    fMessenger = new G4GenericMessenger(this, "/myApp/detector/", "Detector Controls");
    fMessenger->DeclareMethodWithUnit("setMovableAngle", "deg", &DetectorConstruction::SetMovablePMTAngle, "Angle");
    fMessenger->DeclareMethodWithUnit("setFrontDistance", "cm", &DetectorConstruction::SetFrontDistance, "Distance to Front Face");
}

DetectorConstruction::~DetectorConstruction() {
    delete fMessenger;
    if (fMovableRot) delete fMovableRot;
}

void DetectorConstruction::SetMovablePMTAngle(G4double angle) {
    fMovablePMTAngle = angle;
    UpdateDetectorPosition();
}

void DetectorConstruction::SetFrontDistance(G4double dist) {
    fFrontDistance = dist;
    UpdateDetectorPosition();
}

void DetectorConstruction::UpdateDetectorPosition() {
    // PETG 전면 표면 위치 보정값 24.8 mm 반영
    G4double rCenter = fFrontDistance + 24.8 * mm;

    if (fFixedPhys) {
        G4ThreeVector zAxis0(1., 0., 0.);
        fFixedPhys->SetTranslation(rCenter * zAxis0);
    }

    if (fMovablePhys && fMovableRot) {
        G4ThreeVector zAxis1(std::cos(fMovablePMTAngle), 0., std::sin(fMovablePMTAngle));
        G4ThreeVector yAxis(0, 1, 0);
        G4ThreeVector xAxis = yAxis.cross(zAxis1).unit();
        *fMovableRot = G4RotationMatrix(xAxis, yAxis, zAxis1).inverse();
        fMovablePhys->SetTranslation(rCenter * zAxis1);
    }

    G4RunManager::GetRunManager()->GeometryHasBeenModified();
}

G4VPhysicalVolume* DetectorConstruction::Construct() {
    G4NistManager* nist = G4NistManager::Instance();
    G4bool check = true;

    G4Material* air  = nist->FindOrBuildMaterial("G4_AIR");
    G4Material* co60 = nist->FindOrBuildMaterial("G4_Co");

    std::vector<G4double> optEn = {2.0*eV, 2.5*eV, 3.1*eV, 3.5*eV, 4.1*eV};

    G4Material* lsMat = new G4Material("LS_Material", 0.86*g/cm3, 2);
    lsMat->AddElement(nist->FindOrBuildElement("C"), 18);
    lsMat->AddElement(nist->FindOrBuildElement("H"), 30);
    G4MaterialPropertiesTable* mptLS = new G4MaterialPropertiesTable();
    mptLS->AddProperty("RINDEX", optEn, {1.49, 1.49, 1.49, 1.49, 1.49});
    mptLS->AddProperty("ABSLENGTH", optEn, {15.*m, 15.*m, 12.*m, 5.*m, 1.*m});
    mptLS->AddProperty("SCINTILLATIONCOMPONENT1", optEn, {0.01, 0.20, 1.00, 0.40, 0.01});
    mptLS->AddConstProperty("SCINTILLATIONYIELD", 10000./MeV);
    mptLS->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mptLS->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 4.5*ns);
    lsMat->SetMaterialPropertiesTable(mptLS);

    auto logicWorld = new G4LogicalVolume(new G4Box("World", 1*m, 1*m, 1*m), air, "World");
    auto physWorld  = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0, check);
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto logicSource = new G4LogicalVolume(new G4Tubs("Source", 0, 2.5*mm, 0.25*mm, 0, 360*deg), co60, "Source");
    logicSource->SetVisAttributes(new G4VisAttributes(G4Colour::Blue()));
    auto rotSrc = new G4RotationMatrix(); rotSrc->rotateX(90.*deg);
    new G4PVPlacement(rotSrc, G4ThreeVector(), logicSource, "PhysSource", logicWorld, false, 0, check);

    G4LogicalVolume* logicUnit = ConstructDetectorUnit();
    G4double rCenter = fFrontDistance + 24.8 * mm;

    G4ThreeVector zAxis0(1., 0., 0.), yAxis0(0., 1., 0.);
    auto rot0 = new G4RotationMatrix(yAxis0.cross(zAxis0).unit(), yAxis0, zAxis0); rot0->invert();
    fFixedPhys = new G4PVPlacement(rot0, rCenter * zAxis0, logicUnit, "DetectorUnit_0", logicWorld, false, 0, check);

    G4ThreeVector zAxis1(std::cos(fMovablePMTAngle), 0., std::sin(fMovablePMTAngle));
    fMovableRot = new G4RotationMatrix(yAxis0.cross(zAxis1).unit(), yAxis0, zAxis1); fMovableRot->invert();
    fMovablePhys = new G4PVPlacement(fMovableRot, rCenter * zAxis1, logicUnit, "DetectorUnit_1", logicWorld, false, 1, check);

    return physWorld;
}

G4LogicalVolume* DetectorConstruction::ConstructDetectorUnit() {
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    G4Material* glass  = nist->FindOrBuildMaterial("G4_Pyrex_Glass");
    G4Material* tyvek  = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
    G4Material* al     = nist->FindOrBuildMaterial("G4_Al");
    G4Material* petg   = nist->FindOrBuildMaterial("G4_POLYCARBONATE");
    G4Material* pb     = nist->FindOrBuildMaterial("G4_Pb");

    G4Material* grease = new G4Material("Grease", 1.0*g/cm3, 2);
    grease->AddElement(nist->FindOrBuildElement("C"), 1);
    grease->AddElement(nist->FindOrBuildElement("H"), 2);

    std::vector<G4double> optEn = {2.0*eV, 2.5*eV, 3.1*eV, 3.5*eV, 4.1*eV};

    G4MaterialPropertiesTable* mptGlass = new G4MaterialPropertiesTable();
    mptGlass->AddProperty("RINDEX", optEn, {1.47, 1.47, 1.47, 1.47, 1.47});
    mptGlass->AddProperty("ABSLENGTH", optEn, {10.*m, 10.*m, 10.*m, 10.*m, 10.*m});
    glass->SetMaterialPropertiesTable(mptGlass);
    grease->SetMaterialPropertiesTable(mptGlass);

    auto logicUnit = new G4LogicalVolume(new G4Tubs("Unit", 0, 50.0*mm, 30.0*mm, 0, 360*deg), vacuum, "LogicUnit");
    logicUnit->SetVisAttributes(G4VisAttributes::GetInvisible());

    G4double r_ls    = 25.0*mm;
    G4double r_duran = 27.0*mm;
    G4double r_tyvek = 27.2*mm;
    G4double r_al    = 27.3*mm;
    G4double r_petg  = 37.3*mm;
    G4double r_pb    = 42.3*mm;

    // LS 및 부품 배치
    auto logicLS = new G4LogicalVolume(new G4Tubs("LS", 0, r_ls, 12.5*mm, 0, 360*deg), G4Material::GetMaterial("LS_Material"), "LS");
    logicLS->SetVisAttributes(new G4VisAttributes(G4Colour::Red()));
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), logicLS, "PhysLS", logicUnit, false, 0);

    auto logicDuranFront = new G4LogicalVolume(new G4Tubs("DuranFront", 0, r_ls, 1.0*mm, 0, 360*deg), glass, "DuranFront");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, -13.5*mm), logicDuranFront, "PhysDuranFront", logicUnit, false, 0);

    auto logicDuranBack = new G4LogicalVolume(new G4Tubs("DuranBack", 0, r_ls, 1.0*mm, 0, 360*deg), glass, "DuranBack");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, 13.5*mm), logicDuranBack, "PhysDuranBack", logicUnit, false, 0);

    auto logicDuranSide = new G4LogicalVolume(new G4Tubs("DuranSide", r_ls, r_duran, 14.5*mm, 0, 360*deg), glass, "DuranSide");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), logicDuranSide, "PhysDuranSide", logicUnit, false, 0);

    auto logicGrease = new G4LogicalVolume(new G4Tubs("Grease", 0, r_ls, 0.5*mm, 0, 360*deg), grease, "Grease");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, 15.0*mm), logicGrease, "PhysGrease", logicUnit, false, 0);

    auto logicWin = new G4LogicalVolume(new G4Tubs("Win", 0, r_ls, 1.0*mm, 0, 360*deg), glass, "Win");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, 16.5*mm), logicWin, "PhysWin", logicUnit, false, 0);

    auto logicCat = new G4LogicalVolume(new G4Tubs("Cat", 0, r_ls, 0.05*mm, 0, 360*deg), vacuum, "Cat");
    logicCat->SetVisAttributes(new G4VisAttributes(G4Colour::Yellow()));
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, 17.55*mm), logicCat, "PhysCat", logicUnit, false, 0);

    auto logicTyvekFront = new G4LogicalVolume(new G4Tubs("TyvekFront", 0, r_tyvek, 0.1*mm, 0, 360*deg), tyvek, "TyvekFront");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-14.6*mm), logicTyvekFront, "PhysTyvekFront", logicUnit, false, 0);

    auto logicTyvekSide = new G4LogicalVolume(new G4Tubs("TyvekSide", r_duran, r_tyvek, 14.5*mm, 0, 360*deg), tyvek, "TyvekSide");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, 0.0*mm), logicTyvekSide, "PhysTyvekSide", logicUnit, false, 0);

    auto logicAlFront = new G4LogicalVolume(new G4Tubs("AlFront", 0, r_al, 0.05*mm, 0, 360*deg), al, "AlFront");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-14.75*mm), logicAlFront, "PhysAlFront", logicUnit, false, 0);

    auto logicAlSide = new G4LogicalVolume(new G4Tubs("AlSide", r_tyvek, r_al, 14.6*mm, 0, 360*deg), al, "AlSide");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, -0.1*mm), logicAlSide, "PhysAlSide", logicUnit, false, 0);

    auto logicPetgFront = new G4LogicalVolume(new G4Tubs("PetgFront", 0, r_petg, 5.0*mm, 0, 360*deg), petg, "PetgFront");
    logicPetgFront->SetVisAttributes(new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.8)));
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-19.8*mm), logicPetgFront, "PhysPetgFront", logicUnit, false, 0);

    auto logicPetgBack = new G4LogicalVolume(new G4Tubs("PetgBack", 0, r_petg, 1.2*mm, 0, 360*deg), petg, "PetgBack");
    logicPetgBack->SetVisAttributes(new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.8)));
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, 18.8*mm), logicPetgBack, "PhysPetgBack", logicUnit, false, 0);

    auto logicPetgSide = new G4LogicalVolume(new G4Tubs("PetgSide", r_al, r_petg, 16.2*mm, 0, 360*deg), petg, "PetgSide");
    logicPetgSide->SetVisAttributes(new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.5)));
    new G4PVPlacement(nullptr, G4ThreeVector(0,0, 1.4*mm), logicPetgSide, "PhysPetgSide", logicUnit, false, 0);

    auto logicPbSide = new G4LogicalVolume(new G4Tubs("PbSide", r_petg, r_pb, 22.4*mm, 0, 360*deg), pb, "PbSide");
    G4VisAttributes* visPb = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 0.5)); 
    visPb->SetForceWireframe(true);
    logicPbSide->SetVisAttributes(visPb);
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-2.4*mm), logicPbSide, "PhysPbSide", logicUnit, false, 0);

    // 광학 표면 설정
    auto tyvekSurf = new G4OpticalSurface("TyvekSurf", unified, groundfrontpainted, dielectric_dielectric);
    G4MaterialPropertiesTable* mptTyvekSurf = new G4MaterialPropertiesTable();
    mptTyvekSurf->AddProperty("REFLECTIVITY", optEn, {0.98, 0.98, 0.98, 0.98, 0.98}); 
    tyvekSurf->SetMaterialPropertiesTable(mptTyvekSurf);
    new G4LogicalSkinSurface("TyvekSideSkin", logicTyvekSide, tyvekSurf);
    new G4LogicalSkinSurface("TyvekFrontSkin", logicTyvekFront, tyvekSurf);

    auto pmtSurf = new G4OpticalSurface("CathodeSurf", glisur, polished, dielectric_metal);
    G4MaterialPropertiesTable* mptPMT = new G4MaterialPropertiesTable();
    mptPMT->AddProperty("EFFICIENCY", optEn, {0.25, 0.25, 0.25, 0.25, 0.25}); 
    mptPMT->AddProperty("REFLECTIVITY", optEn, {0.0, 0.0, 0.0, 0.0, 0.0});
    pmtSurf->SetMaterialPropertiesTable(mptPMT);
    new G4LogicalSkinSurface("CathodeSkin", logicCat, pmtSurf);

    fLogicPhotocathode = logicCat;
    return logicUnit;
}

void DetectorConstruction::ConstructSDandField() {
    auto sdManager = G4SDManager::GetSDMpointer();
    PMTSD* pmtSD = new PMTSD("PMTSD");
    sdManager->AddNewDetector(pmtSD);
    SetSensitiveDetector(fLogicPhotocathode, pmtSD);
}
