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

```math
\displaylines{
    W = S \delta \overrightarrow{v} 
    \\
    D = -c \overrightarrow{\omega} 
    \\
    R = -k |\overrightarrow{\Delta\theta}|\frac{\overrightarrow{b}_{current} - \overrightarrow{b}_{static}}{|\overrightarrow{b}_{current} - \overrightarrow{b}_{static}|}
    \\
    T = \overrightarrow{bar} \times (W + R + D)
}
```

Where v is the velocity of the wind, S is the area of contact of wind, c and k are damping coefficient and resotration coefficient respectively.

```math
\displaylines{
    \overrightarrow{b}_{current}
    \\
    \overrightarrow{b}_{static}
}
```
These are the static position of the end of the bar that is not connected to the pivot.

![Wrong Restoration Force](./Resources/wrong_restoration_force_1.jpeg "Wrong Restoration Force")

The study uses additional methods such as limiting rotation only about z axis or vertical direction, 
dividing bars into two groups and calculate the bending acceleeration per group instead of per bar.
However, these additional methods are not handled in this project.

### Errors and Physical Considerations Omitted in the Reference Study

#### Errors in Notation
The reference study has some minor notation errors and calculation errors. 
A set of variables in the equation should be either all vectors or scalars, but the author made mistake of mixing both. 
This error hides details of how to exactly. calculate some vectors or scalars.

#### Error in the Torque Calculation

##### Damping force calculation error

Damping force is air friction force.
Air friction force linearly grows with the velocity of the object moving in the air, not its angular velocity.

The correct damping force is $-c (\overrightarrow{\omega} \times \overrightarrow{bar})$

##### Restoration force direction error
The author makes unconventional restoration force vector. 
$$-k |\overrightarrow{\Delta\theta}|\frac{\overrightarrow{b}_{current} - \overrightarrow{b}_{static}}{|\overrightarrow{b}_{current} - \overrightarrow{b}_{static}|}$$

Generally you just write restoration torque equal to $-k \overrightarrow{\Delta\theta}$ and resotration torque are assumed to increase linearly with the angular displacement.

Using 
$$\frac{\overrightarrow{b}_{current} - \overrightarrow{b}_{static}}{|\overrightarrow{b}_{current} - \overrightarrow{b}_{static}|}$$
as a direction of the restoration force,
does not make restoration torque grows linearly with the angular displacement.

Because restoration torque should be $\overrightarrow{bar} \times R$,  
the restoration torque will keep increassing as the angular displacment reaches some point within $[\pi/2, \pi)$ and then decrase to zero at $\pi$.

![Why it is wrong](./Resources/wrong_restoration_force_2.jpeg "Why it is wrong")

The author does not give any justification for this non-monotonic restoration torque. 
There is no explanation such as if this is for reflecting some physical characterestics of grass. 

My conclusion is that this is just a simple mistake made by authors 
for attempting to make restoration torque derived from some force vector as wind torque and damping torques are derived from force vectors.

The correct direction is same as the direction of $\overrightarrow{\Delta\theta} \times \overrightarrow{bar}$, 
but you don't even need to calculate the restoration force vector.

You can just set the restoration torque as $-k \overrightarrow{\Delta\theta}$ and skip the force caculation.


#### Corrected Torque Calculation
```math
\displaylines{
    W = S \delta \overrightarrow{v} 
    \\
    D = -c (\overrightarrow{\omega} \times \overrightarrow{bar})
    \\
    \\
    T = \overrightarrow{bar} \times (W + D) - k \overrightarrow{\Delta\theta}
}
```

Even this corrected equation is ignoring an important physical consideration.


#### Wind and Air friction are not Point Forces

Wind does not push a pivoted bar only at the open end point of the bar.

Wind is pushing the bar along its whole line segment. 
This is same for the air friction.

More physically accurate torque calculation on a straight bar is calculated by itegrating the point torque along the line segment.

If there is a straight bar, and the wind is uniform along the bar's line segment, the torque T is 
```math
\displaylines{
    \overrightarrow{u}_{bar} = \overrightarrow{bar}/|\overrightarrow{bar}|
    \\
    \overrightarrow{W} = S \delta \overrightarrow{v} 
    \\
    \begin{align}
        T & =\int_{0}^{|\overrightarrow{bar}|} (\overrightarrow{W} + -c (\overrightarrow{\omega} \times t\overrightarrow{u}_{bar})) \times tu_{bar}dt - k \overrightarrow{\Delta\theta}
        \\
        & = \frac{|\overrightarrow{bar}|}{2}(\overrightarrow{W} \times \overrightarrow{bar}) - \frac{c|\overrightarrow{bar}|}{3}(\overrightarrow{\omega} \times \overrightarrow{bar} \times \overrightarrow{bar}) - k \overrightarrow{\Delta\theta}
    \end{align} 
}
```

There are quadratic increase of the wind torque and cubic increase of the damping torque with the increase of the bar length. 

![Non Linkage System](./Resources/non_linkage_system.jpeg "Non Linkage System")

If the rotating object is consists of two line segments(noted as bar1 and bar2), the caculation becomes 

```math
\displaylines{
\overrightarrow{u}_{bar2} = \overrightarrow{bar2}/|\overrightarrow{bar2}|
\\
\begin{align}
    T & = \frac{|\overrightarrow{bar1}|}{2}(\overrightarrow{W} \times \overrightarrow{bar1}) - \frac{c|\overrightarrow{bar1}|}{3}(\overrightarrow{\omega}_{bar1} \times \overrightarrow{bar1} \times \overrightarrow{bar1}) - k \overrightarrow{\Delta\theta}_{bar1} 
    \\
    & + \int_{0}^{|\overrightarrow{bar2}|}[(\overrightarrow{W} \times (\overrightarrow{bar1} + t\overrightarrow{u}_{bar2})) - c (\overrightarrow{\omega_{bar1}} \times (\overrightarrow{bar1} + t\overrightarrow{u}_{bar2})) \times (\overrightarrow{bar1} + t\overrightarrow{u}_{bar2}) - c(\overrightarrow{\omega}_{bar2} \times  tu_{bar2})\times  tu_{bar2}]dt
\end{align} 
}
```
(I am not solving the integral term here since the solution have too many terms, but solution is implemented as function `GetBar2TorqueOnP0` in the [GrassMotionShader.ush](Plugins/RotationalDynamicGrass/Shaders/GrassMotionShader.ush)

Above torque calculation is not for two-bars linkage system where two bars are connected by a rotational pivot. 
It is only for a system without a linkage but jst an object with the form of two line segments with fixed connection between those.

With the linkage system things become more complex.

#### Torque from Force on Bars-Linkage System.

If there are multiple bars connected with rotational pivots, a force acting on a bar may not just influence torque on the base of the bar. 
Bellow image shows two example with two-bar linkage system.

![Linkage System Force Examples](./Resources/linkage_system_force_examples.jpeg "Linkage System Force Examples")

As shown in above image if a force actiong on a point of a bar, 
it may create torque with opposite or the same orientation at the other bar depending on the angle between the bars and the force direction.

A force acting on a bar is not guranteed to make a torque on a single pivot.

The reference study omits this physical consideration and calculate each pivot torque independently with each bar. 


