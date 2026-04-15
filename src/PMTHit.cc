#include "PMTHit.hh"

// 멀티스레드 환경을 위한 메모리 풀(Allocator) 초기화
G4ThreadLocal G4Allocator<PMTHit>* PMTHitAllocator = nullptr;
