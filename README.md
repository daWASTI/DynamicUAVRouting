# UGV Navigation in Unknown Terrain (UE5, C++)

This project is a portfolio-focused simulation of an unmanned ground vehicle (UGV) navigating unknown and dynamic terrain, supported by aerial sensing concepts.

The goal is to explore and demonstrate core systems relevant to autonomous navigation, including real-time perception, environment reconstruction, and multi-sensor integration, implemented in Unreal Engine 5 using C++ and Blueprints.

---

## Project Overview

The simulation focuses on how a vehicle can perceive and adapt to an environment that is:
- procedurally generated  
- partially unknown  
- dynamically changing at runtime  

To address this, the project implements a modular sensing pipeline combining simulated LIDAR and vision-inspired classification, which can be fused into a unified representation of the environment.

Blueprints are used for fast prototyping and iteration, while performance-critical systems such as LIDAR processing and navigation logic are implemented in C++.

Note: The vehicle setup is based on Unreal Engine 5’s Vehicle template.

---

## Demonstration

**LIDAR sampling and point cloud visualization**  
![LIDAR](https://github.com/user-attachments/assets/6390f5d0-ce32-4831-b232-a400784ccdb8)

**Real-time mesh reconstruction from sensor data**  
![LIDAR_Mesh](https://github.com/user-attachments/assets/28983fbd-9973-4820-aa92-0e25b4e7eca2)

**Shader-based simulation of ML object classification**

---

## Current Features

- LIDAR simulation using raycast-based environment sampling  
- Real-time point cloud generation and processing (C++)  
- Runtime mesh reconstruction using ProceduralMeshComponent / RealtimeMeshComponent  
- Procedural terrain setup for dynamic, non-deterministic environments  
- Shader-based vision simulation using Custom Stencil with noise (ML approximation)  
- Modular sensor fusion pipeline (in progress)  
- Hybrid Blueprint/C++ architecture for rapid iteration and performance-critical systems  

---

## Planned Features

- Full PCG-based environment generation  
- Destruction events and dynamic obstacles  
- Real-time camera capture for ML-based image recognition  
- Predictive drone paths for optimized environment scanning  
- Adaptive vehicle steering based on reconstructed terrain  

---

## Next Steps

- Optimize point cloud visualization using Niagara Data Channels  
- Integrate LIDAR data fully into mesh generation pipeline  
- Use UE5 navigation systems on reconstructed meshes for autonomous driving
- Benchmark autonomous navigation against human driving (game mode scenario)

---

## Plugins

- RealtimeMeshComponent  
  https://github.com/TriAxis-Games/RealtimeMeshComponent  
