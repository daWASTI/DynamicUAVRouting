# UGV Navigation in Unknown Terrain (UE5, C++)

This project is a portfolio-focused simulation of an unmanned ground vehicle (UGV) navigating unknown and dynamic terrain, supported by aerial sensing concepts.

The goal is to explore and demonstrate core systems relevant to autonomous navigation, including real-time perception, environment reconstruction, and multi-sensor integration, implemented in Unreal Engine 5 using C++ and Blueprints.

---

## Project Overview

The simulation focuses on how a vehicle can perceive and adapt to an environment that is:
- procedurally generated  
- partially unknown  
- dynamically changing at runtime  

To address this, the project implements a modular sensing pipeline that combines simulated LIDAR with vision-inspired classification, with both systems designed to feed a shared environmental understanding over time.

Blueprints are used for fast prototyping and orchestration, while performance-critical systems such as LIDAR tracing, point processing, terrain reconstruction, and runtime visualization are implemented in C++.

The LIDAR stack is structured as a multi-stage runtime pipeline:
- asynchronous line traces sample the environment from the sensor origin  
- raw hit points are visualized as a Niagara point cloud  
- incoming points are buffered and compacted before terrain integration  
- point updates are projected into a local heightfield representation  
- an asynchronous smoothing pass refines only affected mesh regions  
- the resulting surface is pushed into a procedural mesh for downstream navigation and terrain reasoning  

This separation keeps raw sensor visualization independent from the reconstructed terrain surface:
- Niagara shows the direct LIDAR returns  
- the procedural mesh represents a filtered navigable terrain estimate  

Note: The vehicle setup is based on Unreal Engine 5’s Vehicle template.

---

## Demonstration

**LIDAR sampling and point cloud visualization**  
![LIDAR](https://github.com/user-attachments/assets/6390f5d0-ce32-4831-b232-a400784ccdb8)

**Real-time mesh reconstruction from sensor data**  
![LIDAR_Mesh](https://github.com/user-attachments/assets/28983fbd-9973-4820-aa92-0e25b4e7eca2)

**Shader-based approximation of ML object classification**
![VisionML](https://github.com/user-attachments/assets/5bd0f7c8-140e-448c-bc31-eb4215bef517)

---

## Current Features

- Simple randomized landscape and PCG-based environment generation
- LIDAR simulation using async raycast-based environment sampling  
- Raw point cloud visualization through Niagara with per-point color data  
- Real-time terrain reconstruction from buffered LIDAR updates  
- Asynchronous mesh smoothing focused only on affected heightfield regions  
- Runtime mesh reconstruction using ProceduralMeshComponent / RealtimeMeshComponent  
- Procedural terrain setup for dynamic, non-deterministic environments  
- Shader-based vision simulation using Custom Stencil with noise (ML approximation)  
- Modular sensor fusion pipeline (in progress)  
- Hybrid Blueprint/C++ architecture for rapid iteration and performance-critical systems  

---

## Planned Features

- Destruction events and dynamic obstacles   
- Predictive drone paths for optimized environment scanning  
- Adaptive vehicle steering based on reconstructed terrain
- Semantic environment reconstruction (e.g. trees, stone walls)
- Predictive simulation of reconstructed terrain
- Fully autonomous navigation

---

## Next Steps

- Switch to RealtimeMeshComponent for UE5 nav integration
- Potentially switch to fully custom nav if necessary
- Improve Niagara point-cloud handling for cleaner runtime particle management  
- Extend terrain reconstruction beyond the current heightfield-style mesh model  
- Integrate LIDAR-derived terrain fully into autonomous path planning
- Use UE5 navigation systems on reconstructed meshes for autonomous driving
- Benchmark autonomous navigation against human driving (game mode scenario)

---

## Plugins

- RealtimeMeshComponent  
  https://github.com/TriAxis-Games/RealtimeMeshComponent  
