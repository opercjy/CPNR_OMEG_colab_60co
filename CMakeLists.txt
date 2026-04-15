cmake_minimum_required(VERSION 3.16...3.27)
project(Co60_Coincidence_Sim)

# GEANT4 설정
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 및 헤더 지정
file(GLOB SOURCES "src/*.cc")
file(GLOB HEADERS "include/*.hh")
include_directories(${PROJECT_SOURCE_DIR}/include)

# =======================================================
# ★ CRY 우주선 라이브러리 연동 ($CRYHOME 환경변수 사용)
# =======================================================
if(NOT DEFINED ENV{CRYHOME})
    message(FATAL_ERROR "CRYHOME environment variable is not set! Please run 'source /work/cry_v1.7/setup'")
endif()

include_directories($ENV{CRYHOME}/src)
set(CRY_LIB $ENV{CRYHOME}/lib/libCRY.a)

# 실행 파일 생성
add_executable(Co60Sim main.cc ${SOURCES})

# 라이브러리 링크 (Geant4 + CRY)
target_link_libraries(Co60Sim ${Geant4_LIBRARIES} ${CRY_LIB})

# 매크로 파일 복사
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/macros DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
