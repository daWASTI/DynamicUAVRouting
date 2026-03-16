## Dynamic UAV Routing - Simulating navigation through unknown and dynamic terrain
This project is meant as a personal portfolio project and as sandbox for simulating different technical aspects of UAV navigation, such as:
- simulated LIDAR
- real-time mesh generation for navigation
- navigation updates according to dynamic events
- sensor fusion

I'm using Blueprints for rapid iteration and prototyping, while C++ is used for performance-critical systems such as LIDAR processing and navigation logic.

Disclosure: I'm using UE5's "Vehicle" project preset for the car setup.

![LIDAR](https://github.com/user-attachments/assets/6390f5d0-ce32-4831-b232-a400784ccdb8)

# Current Features
- LIDAR simulation
- Setup for Procedural Terrain and LIDAR mesh generation using ProceduralMeshComponent

# Planned Features
- Full PCG environment
- Destruction events and VFX
- Real-time camera capture that can be fed into ML image recognition
- Predictive drone paths for ideal LIDAR mapping
- Adaptive vehicle steering over mapped terrain

# Next Steps
- Optimize point cloud visualization using Niagara Data Channels
- Feed LIDAR data into mesh generation
- Use UE5's built-in navigation for the vehicle based on the LIDAR mesh and use that to steer on the real terrain
- Measure autonomous driving performance vs human driver in a little racing game mode
