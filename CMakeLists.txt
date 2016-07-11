cmake_minimum_required(VERSION 3.5)
project(sampleFilterRaytracer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(RMANTREE $ENV{RMANTREE})
include_directories("${RMANTREE}/include")

add_library(Sphere MODULE Sphere.cpp Sphere.h SampleFilterInstance.h)
add_library(MovingSphere MODULE MovingSphere.cpp MovingSphere.h)
add_library(Perspective MODULE PerspectiveCamera.cpp PerspectiveCamera.h)
add_library(Default MODULE DefaultIntegrator.cpp DefaultIntegrator.h SampleFilterInstance.h)
add_library(Occlusion MODULE OcclusionIntegrator.cpp OcclusionIntegrator.h SampleFilterInstance.h)
add_library(Pathtracer MODULE PathtracerIntegrator.cpp PathtracerIntegrator.h SampleFilterInstance.h)
add_library(Lambert MODULE Lambert.cpp Lambert.h)
add_library(Metal MODULE Metal.cpp Metal.h)

set_target_properties(Sphere MovingSphere Perspective Default Occlusion Pathtracer Lambert Metal PROPERTIES PREFIX "")
