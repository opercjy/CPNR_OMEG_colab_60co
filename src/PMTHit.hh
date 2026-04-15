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

    // EventAction에서 요구하는 3가지 데이터의 Setter/Getter
    void SetModuleID(G4int id) { fModuleID = id; }
    G4int GetModuleID() const  { return fModuleID; }

    void SetTime(G4double t)   { fTime = t; }
    G4double GetTime() const   { return fTime; }

    void SetEdep(G4double e)   { fEdep = e; }
    G4double GetEdep() const   { return fEdep; }

private:
    G4int fModuleID = -1;
    G4double fTime = 0.0;
    G4double fEdep = 0.0;
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
