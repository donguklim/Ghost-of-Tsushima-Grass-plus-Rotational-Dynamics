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
1. 

## Note on Unreal Engine HLSL left-hand rule!
![Left-hand rule rotation](./Resources/left_hand_rule_rotation.jpeg "Left-hand rule rotation")

Unreal Engine 5 is using the **left-hand rule** in its HLSL code and built in quaternion functions.\
Not the conventional right-hand rule.

So given an rotation of axis and a positive angle, the rotation while the axis vector is facing toward you is clockwise direction.

**Positive angle is clockwise direction!**

This is one confusing part of Unreal Engine, because pitch, yaw, roll rotations use counter-clockwise rotation as positive rotation.

This README file is also wrttien in assumption of the using left-hand rule.

(I had spent several hours to figure the problem I had because I didn't know UE was using left-hand rule.)



## Basics Physics of Rotational System

The grass is modeled as a bezier curve controlled by bezier points. 

![Bezier Curve example](./Resources/bezier_curve_example.png "An example of Bezier Curve")

***Quadratic Bezier curve Example drawn from [Demos](https://www.desmos.com/)***

Instead of directly making motion on the bezier curve, the motions are made to the bars(the green line segments in above example) of the bezier curves.


### Forces acting on the bars

There are three types of forces applied to the bars

- Wind force
- Damping force, which is air friction from the angualr velocities of the bars
- Restoration force, which grows linearly with the angular displacement of the bars.

The combined net force makes angular acceleration and updates angular velocity and angular displacement of the bars.

### Torque and Angular Acceleration

![Torque example 1](./Resources/torque_example_1.jpeg "Torque Example 1")

***Torque example 1***


![Torque example 2](./Resources/torque_example_2.jpeg "Torque Example 2")

***Torque example 2***

Given a pivot and an objects that rotates about the pivot, the force acting on the object creates torque.

In above examples the torque $\overrightarrow{T}$ is equal to  $\overrightarrow{F} \times \overrightarrow{B}$ where

```math
\displaylines{
F = \text{Force vector}
\\
B = \text{Vector from pivot point to the point where the force is applied}
\\
}
```
Torque is represented by the cross product of the two vectors. \
The direction of the torque is the axis of the rotation.\
The length of the torque is the magnitude of the torque.

Let T be the torque, the angular acceleration created by the torque is equalt to

```math
\overrightarrow{T} / MI
```

Where MI is **moment of inertia** of the rotating object.

### Moment of Inertia

![Pivot examples](./Resources/pivot_location_examples.jpeg "Pivot examples")

Given a straigt bar with uniform mass density, let
```math
\displaylines{
m = \text{mass of the rotating object}
\\
d = \text{distance from pivot to the center of mass point of the rotating object}
\\
MI_{d} = \text {moment of Inertia with given d}
\\
MIC = MI_{0} = \text {moment of Inertia when the pivot is located at the center of mass of the object}
}
```

Then, by parallel axis theorem,

```math
MI_{d} = MIC + md^{2}

```

For a straight bar with uniform mass density with length L,
```math
MIC = mL^{2}/12
```

And if the bar is rotating about one of its end point, the distance from the pivot to the center of mass is L/2.
Hence,
```math
MI = MI_{L/2} = mL^{2}/12  + m(L/2)^{2} = mL^{2}/3
```

#### MI of objects consists of multiple line segments.
If the rotating object is consists of multiple line segment bars(as drawn on above Torque example 2) and the bars both have uniform mass density, 
then the MI of the object is sum of the each bar MI.

For example, if the object consists of the two line segment bars.
```math
\displaylines{
    m_1 = \text{bar1 mass}
    \\
    m_2 = \text{bar2 mass}
    \\
    \overrightarrow{bar1} = \text{vector from the pivot to the bar1 and bar2 connection point}
    \\
    \overrightarrow{bar2} = \text{vector from bar1 bar2 connection point to the other end of the bar2}
    \\
    \\
    MI = MI_{bar1} + MI_{bar2} = \frac{m_1|\overrightarrow{bar1}|^{2}}{3} + m2(\frac{|\overrightarrow{bar2}|^{2}}{12} + |\overrightarrow{bar1} + \frac{1}{2}\overrightarrow{bar2}|^{2})
}
```


## Rotational Dynamics on Bars-Linkage System Research
Dynamics is used to numerically update the angular velocity and and angular displacement at each time interval between frames.

```math
\displaylines{
    v = \text{angular velocity}
    \\
    d = \text{angular displacement}
    \\
    acc = \text {current angualr acceleration}
    \\
    t\Delta = \text{time delta}
    \\
    \\
    v_{new} = v_{old} + acc \times  t\Delta
    \\
    d_{new} = d_{old} + v_{new} \times t\Delta
}
```

So the problem is accurately calculating the acceleration from the wind force, air friction damping force and grass's restoration force.

### The Reference Study
Because Sucker Punch Studio did not tell how they calculated the grass motion, I have searched if there is any work already done to make physics based grass motion.

Then, I found this article: [A simulation on grass swaying with dynamic wind force](https://link.springer.com/article/10.1007/s00371-016-1263-7)

The study of the article also used Bezier curve modelled grass, and the idea of using rotational physics sounded great, so I decided to make an UE5 implementation of the article. 
At first, my intention was just make an UE5 implementation of the article, without really understanding the physcis theory the dynamics is based on. 

I simply thought I would just copy and paste the equations written in the article.

### Basic equation of the reference study

In the reference study, The wind force, damping force, restoration force and the net torque T acting on the bar are calculated as bellow.

$$
\displaylines{
    W = S \delta \overrightarrow{v} 
    \\
    D = -c\overrightarrow{w} 
    \\
    R = -k |\overrightarrow{\Delta\theta}|(\overrightarrow{b}_{current} - \overrightarrow{b}_{static})
    \\
    T = \overrightarrow{bar} \times (W + R + D)
}
$$

Where v is the velocity of the wind, S is the area of contact of wind, c and k are damping coefficient and resotration coefficient respectively.

$$
\displaylines{
    \overrightarrow{b}_{current}
    \\
    \overrightarrow{b}_{static}
}
$$
$\overrightarrow{b}_{current}$ and $\overrightarrow{b}_{static}$ are the static position of the end of the bar that is not connected to the pivot.

### Errors and Physical Considerations Omitted in the Baseline Study

#### Errors
The reference study has some minor notation errors and calculation errors.

- Some set of variables should be either vectors or scalars, but the authors mix those together. Confusing the readers and does not tell how to correctly calculate some values.
-

- some set of variables in the quation must be all either vector variables or scalar variables, but the authors seemd to mixed both by mistake.
- It is a simple error, but this hides how to actually calculate the damping force value.
- It was a simple error, but I had to study some physics to fix the error.

#### Treating wind and damping forces as point force

The authors of made calculation as if the wind and air damping 

#### Incorrect Damping Force and Restoration Force Calculation

#### Ignoring influence between the pivots