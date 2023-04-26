# CRT: OpenCL Raytracer
This project is a mishmash of Peter Shirley's fantastic [raytracing
books](https://raytracing.github.io/), and Jacco Bikker's equally fantastic
[BVH building
series](https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/).
This project aims to implement a raytracer that supports multiple primitives
(currently only spheres, triangles planned in the future, among others), more
advanced materials, and more advanced techniques than what's in the
aforementioned sources (Importance sampling for example).

# How to build
You should have OpenCL installed on your computer, and a C++17 compiler (tested
on gcc), otherwise, this project is self contained. You can just run
`build_release.sh`.

This project has been tested on an AMD GPU (Vega 6 iGPU) and Nvidia GPU (GTX
1660Ti).

# Gallery
![Random spheres, the last scene of `Raytracing in one weekend`](screenshots/random_spheres.jpg)
![Two spheres, a scene that shows off checkered textures. Chapter 4 of `Raytracing, the next weekend`](screenshots/two_checkered_spheres.jpg)
