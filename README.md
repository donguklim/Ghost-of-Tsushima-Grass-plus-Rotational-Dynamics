# UE5 Implementation of Rotational Dynamics Grass 

## Introduction

This is an implementation of the Ghost of Tushima grass + rotational physics dynamics based motion of Bezier curve grass.

The implementation of the GOT grass is based on the [GDC presentation](https://www.youtube.com/watch?v=Ibe1JBF5i5Y) of the Sucker Punch Studio.
The presenter of the Sucker Punch Studio did not tell how they calculated the grass motion, so I have made my own rotational physics based dynamics motion.


## General Structure of the Grass Generation and Motion

PCG and Niagara are used to spawn the grasses and calculate the motions of each grass instances.

In order to combine PCG with Niagara, I am using a beta status plugin made by Epic Games, which provides a PCG nodes that writes data to Niagara data channel.
  
### PCG
1. Partitioned PCG generates grasses with hierarchial grids.
    - Grasses are generated at runtime.
    - Larger grids in higher hierarchy generates fewer grasses than smaller grids in lower hierarchy. 
    - Larger grids have longer generation radius and clean up radius than the smaller grids. Making fewer grasses generation at distant area and more grasses in near area.
2. PCG generates grasses by writing data to Niagara Data Channel.
    - Write grass instance data to NDC as a PCG point enters within the generation radius.
    - Write the grass instance data again if the PCG point gets out of the clean up radius and re-enters to the generation radius.
   

### Niagara
1. Spawn grass instances as data is received from Niagara Data Channel
2. Calculate and update the motion of grass instances.
3. Clean up a grass instance if it needs to be cleaned up.

## Rotational Dynamics Based Motion.

The grass is modeled as a bezier curve controlled by bezier points. 

![Bezier Curve example](./Resources/bezier_curve_example.png "An example of Bezier Curve")

(Quadratic Bezier curve Example drawn from [Demos](https://www.desmos.com/))

Instead of directly making motion on the bezier curve, the motions are made to the bars(the green line segments in above example) of the bezier curves.

### Forces acting on the bars

There are three types of forces applied to the bars

- Wind force
- Damping force, which is air friction from the angualr velocities of the bars
- Restoration force, which grows linearly with the angular displacement of the bars.

The combined net force makes angular acceleration and updates angular velocity and angular displacement of the bars.

### Dynamics method
Dynamics is used to numerically update the angular velocity and and angular displacement at each time interval between frames.

```math
\displaylines{
v = \text{angular velocity}
\\
d = \text{angular displacement}
\\
acc = \text {current angualr acceleration}
<br>
\\
t\Delta = \text{time delta}
\\
\\
v_{new} = v_{old} + acc * t\Delta
\\
d_{new} = d_{old} + v_{new} * t\Delta
}
```

I will explain how the angular acceleration is calculated in the later section of this readme.

### At the Beginning
Because Sucker Punch Studio did not tell how they calculated the grass motion, so I have searched if there is any work already done to make physics based grass motion.

Then, I found this article: [A simulation on grass swaying with dynamic wind force](https://link.springer.com/article/10.1007/s00371-016-1263-7)

The article also used Bezier curve modelled grass, and the idea of using rotational physics sounded great, so I decided to make an UE5 implementation of the article. 
At first, my intention was just make an UE5 implementation of the article, without really understanding the physcis theory the dynamics is based on. 

I simply thought I would just copy and paste the equations written in the article.

### But the equations have flaws I couldn't overlook.

1. Equations have errors of mixing scalra variables and vector variables.
    - some set of variables in the quation must be all either vector variables or scalar variables, but the authors seemd to mixed both by mistake.
    - It is a simple error, but this hides how to actually calculate the damping force value.
