#include "G4Types.hh"
#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "MyPhysicsList.hh"

int main(int argc, char** argv) {
    G4UIExecutive* ui = nullptr;
    if (argc == 1) {
        ui = new G4UIExecutive(argc, argv);
    }

    auto runManager = G4RunManagerFactory::CreateRunManager(
        (ui) ? G4RunManagerType::SerialOnly : G4RunManagerType::Default
    );

    runManager->SetUserInitialization(new DetectorConstruction());
    runManager->SetUserInitialization(new MyPhysicsList());
    runManager->SetUserInitialization(new ActionInitialization());

    G4VisManager* visManager = new G4VisExecutive;
    visManager->Initialize();
    G4UImanager* UImanager = G4UImanager::GetUIpointer();

    if (!ui) {
        // 배치 모드 (macro 실행)
        G4String command = "/control/execute ";
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);

        // ★ 에러를 뱉던 hadd 자동 병합 로직을 완전히 삭제했습니다.
        G4cout << "\n=========================================================" << G4endl;
        G4cout << "=== Geant4 Simulation Finished Successfully! ===" << G4endl;
        G4cout << "=========================================================\n" << G4endl;
    } else {
        // GUI 모드
        UImanager->ApplyCommand("/control/execute macros/init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;
    return 0;
}
