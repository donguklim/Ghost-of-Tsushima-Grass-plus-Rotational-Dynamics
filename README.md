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

## Note on Unreal Engine HLSL left-hand rule!
![Left-hand rule rotation](./Resources/left_hand_rule_rotation.jpeg "Left-hand rule rotation")

Unreal Engine 5 is using the **left-hand rule** in its HLSL code and built in quaternion functions.\
Not the conventional right-hand rule.

So given an rotation of axis and a positive angle, the rotation while the axis vector is facing toward you is clockwise direction.

**Positive angle is clockwise direction!**

This is one confusing part of Unreal Engine, because pitch, yaw, roll rotations use counter-clockwise rotation as positive rotation.

This README file is also wrttien in assumption of the using left-hand rule.

(I had spent several hours to figure the problem I had because I didn't know UE was using left-hand rule.)



## Basics Physics of Bars Rotational System

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


## The Reference Study
Because Sucker Punch Studio did not tell how they calculated the grass motion, I have searched if there is any work already done to make physics based grass motion.

Then, I found this article: [A simulation on grass swaying with dynamic wind force](https://link.springer.com/article/10.1007/s00371-016-1263-7)

The study of the article also used Bezier curve modelled grass, and the idea of using rotational physics sounded great, so I decided to make an UE5 implementation of the article. 
At first, my intention was just make an UE5 implementation of the article, without really understanding the physcis theory the dynamics is based on. 

I simply thought I would just copy and paste the equations written in the article.

Note: The study uses the term "edge" to refer the bars in the bars-linkage rotational system, but in physics studies that involves a rotational system "bar" is a term more often used. 
Hence, I am also going to use this term.

### Use of Dynamics
The reference study suggests to use dynamics that numerically updates the angular velocity and and angular displacement at each time interval.

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

So the main problem is calculating plausible angular acceleration from the wind force, air friction damping force and grass's restoration force.


### The Basic Equation of the Reference Study

In the reference study, The wind force, damping force, restoration force acting on a bar and the net torque T applied to the pivot are calculated as bellow.

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

![Inconsistent Restoration Force](./Resources/inconsistent_restoration_force_1.jpeg "Inconsistent Restoration Force")

### Other Methods in Reference Study.

The reference study tells that applying above calculation to each individual bar and making full 3-dimensional rotation gives inconsistent motions between adjacent bars.
So it uses additional methods.

#### Limiting the Rotational Axis.
![Grass Direction Vecotrs](./Resources/grass_direction_vectors.jpeg "Grass Direction Vecotrs")
The pivot at ground can only roatate about the land normal and $E_w$ vector direction of the grass blase(shown in the above image).

The rotation about land normal is referred as swinging and the rotation about $E_w$ is referred as bending.

Other non-ground pivots that connect the bars are only limited to make bending.


#### Grouped Bending and Swinging

Once the swinging angular displacement of a grass instance is calculated, all pivot points are swinged with certain ratio of the calculated angular displacement. 
The ratio depends on the stiffness of each bars of the grass with the bar connected to the grounds have the strongest stiffness and the stiffness decreases as the bars are near to the tip of the grass.
Force acting on the tip of the grass is used for swinging torque calculation.

Bending also uses the same method, but unlike swing where there is only a single angualr displacement shared by pivots, 
bending divides pivots into at most two groups and calculates the bending angular displacement for each group.
The force at the furthest tip of each group is used for torque calculation of each group.

This method is not used in this study, so I omit detailed explanation.
Read the original paper for the further details. 


#### Samping The wind force only once for each grass

Wind force is sampled once for each grass instance. So the bars of one grass instance receives uniform wind force.


#### Methods adopted in this study.

Limiting non-ground pivot's rotational axis $E_w$ vector, and samping wind force once for each grass instance is used in this study.
The wind is sampled once for each grass for faster calculation.
The ground pivot is rendered as a full 3 dimensional pivot. 


### Errors and Physical Considerations Omitted in the Reference Study

#### Errors in Notation
The reference study has some minor notation errors and calculation errors. 
A set of variables in the equation should be either all vectors or scalars, but the author made mistake of mixing both. 
This error hides details of how to exactly. calculate some vectors or scalars.

The equation shown in `Basic Equation of the Reference Study` section is a notation corrected version.

#### Error in the Torque Calculation

##### Damping force calculation error

Damping force is air friction force.
Air friction force linearly grows with the actual velocity of the object moving in the air, not its angular velocity.

The correct damping force vector is $-c (\overrightarrow{\omega} \times \overrightarrow{bar})$

##### Inconsistent Restoration force direction
The authors make unconventional restoration force vector. 
```math
-k |\overrightarrow{\Delta\theta}| \frac{\overrightarrow{b}_{current} - \overrightarrow{b}_{static}}{|\overrightarrow{b}_{current} - \overrightarrow{b}_{static}|}
```
Generally restoration torque is considered to be $-k \overrightarrow{\Delta\theta}$

Resotration torque is assumed to increase linearly with the angular displacement.

Using 
```math
\frac{\overrightarrow{b}_{current} - \overrightarrow{b}_{static}}{|\overrightarrow{b}_{current} - \overrightarrow{b}_{static}|}
```
as a direction of the restoration force, does not make restoration torque grows linearly with the angular displacement.

With the restoration force in the reference study, the restoration torque becomes $\overrightarrow{bar} \times R$.

It becomes non-monotonic with the angular displacement. 

The restoration torque will keep increassing as the angular displacment reaches to some angle within $[\pi/2, \pi)$ and then decrase to zero at $\pi$.

![Why it is inconsistent](./Resources/inconsistent_restoration_force_2.jpeg "Why it is inconsistent")

The author does not give any justification for this unconventional non-monotonic behavior restoration torque. 
The authors does not explain if this is for reflecting some physical characterestics of grass. 

It seems the authors simply made mistake for attempting to make restoration torque derived from some force vector as wind torque and damping torques are derived from force vectors.

The correct direction of the restoration force should be same as the direction of $\overrightarrow{\Delta\theta} \times \overrightarrow{bar}$, 
but this force calculation is unncessary as the restoration torque can be simply calculated as $-k \overrightarrow{\Delta\theta}$.


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
        T & =\int_{0}^{|\overrightarrow{bar}|} (\overrightarrow{W} - c (\overrightarrow{\omega} \times t\overrightarrow{u}_{bar})) \times tu_{bar}dt - k \overrightarrow{\Delta\theta}
        \\
        & = \frac{|\overrightarrow{bar}|}{2}(\overrightarrow{W} \times \overrightarrow{bar}) - \frac{c|\overrightarrow{bar}|}{3}(\overrightarrow{\omega} \times \overrightarrow{bar} \times \overrightarrow{bar}) - k \overrightarrow{\Delta\theta}
    \end{align} 
}
```

There are quadratic increase of the wind torque and cubic increase of the damping torque with the increase of the bar length.

This difference in the degree of the bar length in the torque equation could be considered as a type of approximation.

However, treating wind and air friction as point forces gives more distortion when it is combined with the grouped torque calculation method in the reference study.

![Distorted Swing](./Resources/distorted_swing.jpeg "Distorted Swing")

The reference study calculates swing torque with the force applied at the tip of the grass. 

In a case like above image, the torque will be the force at the tip and the red arrow, 
which is the vector from the ground pivot to the point where the swinging force is applied.

The force applied at the middle pivot point(pointed by the blue arrow) is ignored in swinging torque calculation 
even though it is more distant from the ground pivot than the tip.

Same problem also applies to the bendings.

##### Torque calculation with Two line segments

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
(I am not solving the integral term here since the solution have too many terms, but solution is implemented as function `GetBar2TorqueOnP0` in the shader file [Plugins/RotationalDynamicGrass/Shaders/GrassMotionShader.ush](Plugins/RotationalDynamicGrass/Shaders/GrassMotionShader.ush)

Above torque calculation is not for two-bars linkage system where two bars are connected by a rotational pivot. 
It is only for a system without a linkage but jst an object with the form of two line segments with fixed connection between those.

With the linkage system torque caculation becomes more complex.

#### Torque from Force on Bars-Linkage System.

If there are multiple bars connected with rotational pivots, a force acting on a bar may not just influence torque on the base of the bar. 
Bellow image shows two example with two-bar linkage system.

![Linkage System Force Examples](./Resources/linkage_system_force_examples.jpeg "Linkage System Force Examples")

As shown in above image if a force actiong on a point of a bar, 
it may create torque with opposite or the same orientation at the other bar depending on the angle between the bars and the force direction.

A force acting on a bar is not guranteed to make a torque on a single pivot.

The reference study omits this physical consideration and calculate each pivot torque independently with each bar. 


## Methods Used in This Study.


### Payback Method


