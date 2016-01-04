# Watersimulation

In this project a water-simulation was implemented using Navier-Stokes equations. 
Decent effort was invested to make the result visually pleasing.

Everything is rendered with OpenGL 4.3 at real-time frame rates applying a simple ad-hoc shading. 
Multiple rendering passes are used to determine the depth of water and calculating fake refractions of the ground. 
Light is attenuated by the water based on the depth from the point of view of the light source. 
To give the impression of subsurface-scattering and shift in color a look-up texture is used.
Caustics are achieved by projecting a texture onto the ground. Nothing real here either.
To give the impression of small ripples on the water a normals are read from a normal-map.

The water-simulation is implemented in the compute shader.

![alt tag](https://github.com/thehenkk/Watersimulation/blob/master/images/1.jpg)

To make the simulation interesting the water is pulled up every few seconds by two gaussian bells.
The normals are recalculated after each simulation step, which is visualized with the aid a geometry shader.

![alt tag](https://github.com/thehenkk/Watersimulation/blob/master/images/2.jpg)

