#pragma once
#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "tls.hh"

class PMTHit : public G4VHit {
public:
    PMTHit() = default;
    ~PMTHit() override = default;

    inline void* operator new(size_t);
    inline void  operator delete(void*);

    void SetModuleID(G4int id) { fModuleID = id; }
    G4int GetModuleID() const  { return fModuleID; }

    // ★ [O(1) 최적화] 개별 광자 할당을 버리고 누적형(Accumulative) 메소드로 전환
    void AddPE() { fPE++; }
    G4int GetPE() const { return fPE; }

    void UpdateTime(G4double t) {
        if (fPE == 0 || t < fTimeFirst) fTimeFirst = t;
        if (fPE == 0 || t > fTimeLast)  fTimeLast = t;
    }
    G4double GetTimeFirst() const { return fTimeFirst; }
    G4double GetTimeLast() const  { return fTimeLast; }

    void SetSourceType(G4int s) { fSourceType = s; }
    G4int GetSourceType() const { return fSourceType; }

private:
    G4int fModuleID = -1;
    G4int fPE = 0;
    G4double fTimeFirst = 0.0;
    G4double fTimeLast = 0.0;
    G4int fSourceType = -1;
};

using PMTHitsCollection = G4THitsCollection<PMTHit>;
extern G4ThreadLocal G4Allocator<PMTHit>* PMTHitAllocator;

inline void* PMTHit::operator new(size_t) {
    if (!PMTHitAllocator) PMTHitAllocator = new G4Allocator<PMTHit>;
    return (void*)PMTHitAllocator->MallocSingle();
}

inline void PMTHit::operator delete(void* hit) {
    PMTHitAllocator->FreeSingle((PMTHit*)hit);
}
