# UE5 Implementation of Rotational Dynamics Grass 

## Introduction

This is an implementation of the Ghost of Tsushima grass + rotational physics dynamics based motion of Bezier curve grass.

The implementation of the GOT grass is based on the [GDC presentation](https://www.youtube.com/watch?v=Ibe1JBF5i5Y) of Sucker Punch Studio.
The presenter from Sucker Punch Studio did not explain how they calculated the grass motion, so I have created my own rotational physics based dynamics motion.

## General Structure of the Grass Generation and Motion

PCG and Niagara are used to spawn the grasses and calculate the motions of each grass instance.

In order to combine PCG with Niagara, I am using a beta status plugin made by Epic Games, which provides PCG nodes that write data to Niagara data channels.
  
### PCG
1. Partitioned PCG generates grasses with hierarchical grids.
    - Grasses are generated at runtime.
    - Larger grids in higher hierarchy generate fewer grasses than smaller grids in lower hierarchy. 
    - Larger grids have longer generation radius and clean up radius than the smaller grids, making fewer grass generations at distant areas and more grasses in near areas.
2. PCG generates grasses by writing data to Niagara Data Channel.
    - Write grass instance data to NDC as a PCG point enters within the generation radius.
    - Write the grass instance data again if the PCG point gets out of the clean up radius and re-enters the generation radius.
   
### Niagara
1. Spawn grass instances as data is received from Niagara Data Channel
2. Calculate and update the motion of grass instances.
3. Clean up a grass instance if it needs to be cleaned up. 

## Note on Unreal Engine HLSL left-hand rule!
![Left-hand rule rotation](./Resources/left_hand_rule_rotation.jpeg "Left-hand rule rotation")

Unreal Engine 5 uses the **left-hand rule** in its HLSL code and built-in quaternion functions,\
not the conventional right-hand rule.

So given a rotation of axis and a positive angle, the rotation while the axis vector is facing toward you is in the clockwise direction.

**Positive angle is clockwise direction!**

This is one confusing part of Unreal Engine, because pitch, yaw, roll rotations use counter-clockwise rotation as positive rotation.

This README file is also written with the assumption of using the left-hand rule.

(I spent several hours figuring out the problem I had because I didn't know UE was using the left-hand rule.)

## Basic Physics of Bars Rotational System

The grass is modeled as a bezier curve controlled by bezier points. 

![Bezier Curve example](./Resources/bezier_curve_example.png "An example of Bezier Curve")

***Quadratic Bezier curve Example drawn from [Demos](https://www.desmos.com/)***

Instead of directly making motion on the bezier curve, the motions are applied to the bars (the green line segments in the above example) of the bezier curves.

### Forces acting on the bars

There are three types of forces applied to the bars:

- Wind force
- Damping force, which is air friction from the angular velocities of the bars
- Restoration force, which grows linearly with the angular displacement of the bars.

The combined net force creates angular acceleration and updates angular velocity and angular displacement of the bars.

### Torque and Angular Acceleration

![Torque example 1](./Resources/torque_example_1.jpeg "Torque Example 1")

***Torque example 1***

![Torque example 2](./Resources/torque_example_2.jpeg "Torque Example 2")

***Torque example 2***

Given a pivot and an object that rotates about the pivot, the force acting on the object creates torque.

In the above examples, the torque $\overrightarrow{T}$ is equal to $\overrightarrow{F} \times \overrightarrow{B}$ where

```math
\displaylines{
F = \text{Force vector}
\\
B = \text{Vector from pivot point to the point where the force is applied}
\\
}
```
Torque is represented by the cross product of the two vectors.\
The direction of the torque is the axis of the rotation.\
The length of the torque is the magnitude of the torque.

Let T be the torque, the angular acceleration created by the torque is equal to

```math
\overrightarrow{T} / MI
```

Where MI is **moment of inertia** of the rotating object.

### Moment of Inertia

![Pivot examples](./Resources/pivot_location_examples.jpeg "Pivot examples")

Given a straight bar with uniform mass density, let
```math
\displaylines{
m = \text{mass of the rotating object}
\\
d = \text{distance from pivot to the center of mass point of the rotating object}
\\
MI_{d} = \text{moment of Inertia with given d}
\\
MIC = MI_{0} = \text{moment of Inertia when the pivot is located at the center of mass of the object}
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

And if the bar is rotating about one of its end points, the distance from the pivot to the center of mass is L/2.
Hence,
```math
MI = MI_{L/2} = mL^{2}/12 + m(L/2)^{2} = mL^{2}/3
```

#### MI of objects consisting of multiple line segments

If the rotating object consists of multiple line segment bars (as drawn in the above Torque example 2) and the bars both have uniform mass density, 
then the MI of the object is the sum of each bar's MI.

For example, if the rotating object consists of the two line segment bars,
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

Because Sucker Punch Studio did not explain how they calculated the grass motion, I searched for any existing work on physics-based grass motion.

I found this article: [A simulation on grass swaying with dynamic wind force](https://link.springer.com/article/10.1007/s00371-016-1263-7)

The study in the article also used Bezier curve modeled grass, and the idea of using rotational physics sounded great, so I decided to make a UE5 implementation of the article. 
At first, my intention was just to make a UE5 implementation of the article without really understanding the physics theory the dynamics is based on. 

I simply thought I would just copy and paste the equations written in the article.

Note: The study uses the term "edge" to refer to the bars in the bar-linkage rotational system, but in physics studies that involve a rotational system, "bar" is a term more often used. 
Hence, I am also going to use this term.

### Use of Dynamics

The reference study suggests using dynamics that numerically updates the angular velocity and angular displacement at each time interval.

```math
\displaylines{
    v = \text{angular velocity}
    \\
    d = \text{angular displacement}
    \\
    acc = \text{current angular acceleration}
    \\
    t\Delta = \text{time delta}
    \\
    \\
    v_{new} = v_{old} + acc \times  t\Delta
    \\
    d_{new} = d_{old} + v_{new} \times t\Delta
}
```

So the main problem is calculating plausible angular acceleration from the wind force, air friction damping force, and grass's restoration force.


### The Basic Equation of the Reference Study

In the reference study, the wind force, damping force, restoration force acting on a bar, and the net torque T applied to the pivot are calculated as below.

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

Where v is the velocity of the wind, S is the area of contact of wind, $\delta$ is wind force coefficient, c and k are damping coefficient and restoration coefficient respectively.

```math
\displaylines{
    \overrightarrow{b}_{current}
    \\
    \overrightarrow{b}_{static}
}
```
These are the static position of the end of the bar that is not connected to the pivot.

The author of the reference study assumes wind force to be linear with wind velocity v, but actually wind force grows linearly with $v^2$ in physics.
In this document, the term "wind force" will be used instead of $\delta \overrightarrow{v}$.

![Inconsistent Restoration Force](./Resources/inconsistent_restoration_force_1.jpeg "Inconsistent Restoration Force")

### Other Methods in Reference Study

The reference study states that applying the above calculation to each individual bar and making full 3-dimensional rotation gives inconsistent motions between adjacent bars.
So it uses additional methods.

#### Limiting the Rotational Axis

![Grass Direction Vectors](./Resources/grass_direction_vectors.jpeg "Grass Direction Vectors")

The pivot at ground can only rotate about the land normal and $\overrightarrow{E}_w$ vector direction of the grass blade (shown in the above image).

The rotation about land normal is referred to as swinging and the rotation about $\overrightarrow{E}_w$ is referred to as bending.

Other non-ground pivots that connect the bars are only limited to making bending.


#### Grouped Bending and Swinging

Once the swinging angular displacement of a grass instance is calculated, all pivot points are swung with a certain ratio of the calculated angular displacement. 
The ratio depends on the stiffness of each bar of the grass, with the bar connected to the ground having the strongest stiffness, and the stiffness decreases as the bars are near the tip of the grass.
Force acting on the tip of the grass is used for swinging torque calculation.

Bending also uses the same method, but unlike swinging where there is only a single angular displacement shared by pivots, 
bending divides pivots into at most two groups and calculates the bending angular displacement for each group.
The force at the furthest tip of each group is used for torque calculation of each group.

This method is not used in this study, so I omit detailed explanation.
Read the original paper for further details. 


#### Sampling The Wind Force Only Once for Each Grass

Wind force is sampled once for each grass instance. So the bars of one grass instance receive uniform wind force.


#### Methods Adopted in This Study

Limiting non-ground pivot's rotational axis to the $E_w$ vector, and sampling wind force once for each grass instance are used in this study.
The wind is sampled once for each grass for faster calculation.
The ground pivot is rendered as a full 3-dimensional pivot. 


### Errors and Physical Considerations Omitted in the Reference Study

#### Errors in Notation
The reference study has some minor notation errors and calculation errors. 
A set of variables in the equation should be either all vectors or scalars, but the author made the mistake of mixing both. 
This error hides details of how to exactly calculate some vectors or scalars.

The equation shown in the `Basic Equation of the Reference Study` section is a notation-corrected version.

#### Error in the Torque Calculation

##### Damping Force Calculation Error

Damping force is air friction force.
Air friction force linearly grows with the actual velocity of the object moving in the air, not its angular velocity.

The correct damping force vector is $-c (\overrightarrow{\omega} \times \overrightarrow{bar})$

##### Inconsistent Restoration Force Direction
The authors make an unconventional restoration force vector. 
```math
-k |\overrightarrow{\Delta\theta}| \frac{\overrightarrow{b}_{current} - \overrightarrow{b}_{static}}{|\overrightarrow{b}_{current} - \overrightarrow{b}_{static}|}
```
Generally, restoration torque is considered to be $-k \overrightarrow{\Delta\theta}$

Restoration torque is assumed to increase linearly with the angular displacement.

Using 
```math
\frac{\overrightarrow{b}_{current} - \overrightarrow{b}_{static}}{|\overrightarrow{b}_{current} - \overrightarrow{b}_{static}|}
```
as a direction of the restoration force does not make restoration torque grow linearly with the angular displacement.

With the restoration force in the reference study, the restoration torque becomes $\overrightarrow{bar} \times R$.

It becomes non-monotonic with the angular displacement. 

The restoration torque will keep increasing as the angular displacement reaches some angle within $[\pi/2, \pi)$ and then decrease to zero at $\pi$.

![Why it is inconsistent](./Resources/inconsistent_restoration_force_2.jpeg "Why it is inconsistent")

The authors do not give any justification for this unconventional non-monotonic behavior of restoration torque. 
They do not explain if this is for reflecting some physical characteristics of grass. 

It seems the authors simply made a mistake by attempting to make restoration torque derived from some force vector, as wind torque and damping torques are derived from force vectors.

The correct direction of the restoration force should be the same as the direction of $\overrightarrow{\Delta\theta} \times \overrightarrow{bar}$, 
but this force direction calculation is unnecessary as the restoration torque can be simply calculated as $-k \overrightarrow{\Delta\theta}$.


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

Even this corrected equation ignores an important physical consideration.


#### Wind and Air Friction are not Point Forces

Wind does not push a pivoted bar only at the open end point of the bar.

Wind is pushing the bar along its whole line segment. 
This is the same for air friction.

A more physically accurate torque calculation on a straight bar is calculated by integrating the point torque along the line segment.

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

There is quadratic increase of the wind torque and cubic increase of the damping torque with the increase of the bar length.

This difference in the degree of the bar length in the torque equation could be considered as a type of approximation.

However, treating wind and air friction as point forces gives more distortion when it is combined with the grouped torque calculation method in the reference study.

![Distorted Swing](./Resources/distorted_swing.jpeg "Distorted Swing")

The reference study calculates swing torque with the force applied at the tip of the grass. 

In a case like the above image, the torque will be the force at the tip and the red arrow, 
which is the vector from the ground pivot to the point where the swinging force is applied.

The force applied at the middle pivot point (pointed by the blue arrow) is ignored in swinging torque calculation 
even though it is more distant from the ground pivot than the tip.

The same problem also applies to bendings.

##### Torque Calculation with Two Line Segments

![Non Linkage System](./Resources/non_linkage_system.jpeg "Non Linkage System")

If the rotating object consists of two line segments (noted as bar1 and bar2), the calculation becomes 

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
(I am not solving the integral term here since the solution has too many terms, but the solution is implemented as function `GetBar2TorqueOnP0` in the shader file [Plugins/RotationalDynamicGrass/Shaders/GrassMotionShader.ush](Plugins/RotationalDynamicGrass/Shaders/GrassMotionShader.ush)

The above torque calculation is not for a two-bars linkage system where two bars are connected by a rotational pivot. 
It is only for a system without a linkage but just an object with the form of two line segments with fixed connection between them.

With the linkage system, torque calculation becomes more complex.

#### Feedbacks Between Bars in Bar-Linkage System

If there are multiple bars connected with rotational pivots, a force acting on a bar may not just influence torque on the base pivot of the bar. 
The image below shows two examples with a two-bar linkage system.

![Linkage System Force Examples](./Resources/linkage_system_force_examples.jpeg "Linkage System Force Examples")

As shown in the above image, a force acting on a point of a bar may create torque with opposite or the same orientation to another bar 
depending on the angle between the bars and the force direction.

A force acting on a bar is not guaranteed to make a torque on the single pivot at the base of the bar. 

There are feedbacks between bars.

The reference study omits this physical consideration and calculates each pivot torque independently with each bar. 


## Methods Used in This Motion Study

The basic idea of using rotational system dynamics is the same as the reference study.

Limitation of rotation to bending on non-ground pivots and single wind force sampling per grass instance from the reference study are also used.

Limitation of rotation to bending is used for more plausible motion. 
Single wind sampling per grass instance is used for faster computation.

However, the torque calculation differs from the reference study as wind and air friction forces are applied to line segments 
and an approximation method is used for reflecting physical characteristics of the bar-linkage systems.

Also, there are additional methods for preventing distorted motions.

The methods in this study are implemented with a two-bars linkage system. A Quadratic Bezier curve is used to render the grasses in the implementation.


### Payback Torque Calculation Method

A calculation method which will be referred to as the 'payback method' is devised in this study to account for the feedbacks between bars.

The method will be explained with a two-bar linkage system example.

A two-bar linkage system with bar1, bar2, stationary pivot P0 and another pivot P1 that connects the bars are given.


![Payback example 01](./Resources/payback_example_01.jpeg "Payback example 01")

#### 1. Calculate P1 Seized P0 Angular Acceleration

Calculate the angular acceleration $acc_{0fixed}$ that would occur at P0 if P1 did not exist and bar1 and bar2 have a fixed connection.

![Payback example 02](./Resources/payback_example_02.jpeg "Payback example 02")

```math
\displaylines{
\overrightarrow{u}_{bar2} = \overrightarrow{bar2}/|\overrightarrow{bar2}|
\\
 m_1 = \text{bar1 mass}
 \\
 m_2 = \text{bar2 mass}
 \\
 \\
 MI = MI_{bar1} + MI_{bar2} = \frac{m_1|\overrightarrow{bar1}|^{2}}{3} + m2(\frac{|\overrightarrow{bar2}|^{2}}{12} + |\overrightarrow{bar1} + \frac{1}{2}\overrightarrow{bar2}|^{2})
\\
\\
\begin{align}
    T & = \frac{|\overrightarrow{bar1}|}{2}(\overrightarrow{W} \times \overrightarrow{bar1}) - \frac{c|\overrightarrow{bar1}|}{3}(\overrightarrow{\omega}_{bar1} \times \overrightarrow{bar1} \times \overrightarrow{bar1}) - k_{p0} \overrightarrow{\Delta\theta}_{bar1} 
    \\
    & + \int_{0}^{|\overrightarrow{bar2}|}[(\overrightarrow{W} \times (\overrightarrow{bar1} + t\overrightarrow{u}_{bar2})) - c (\overrightarrow{\omega_{bar1}} \times (\overrightarrow{bar1} + t\overrightarrow{u}_{bar2})) \times (\overrightarrow{bar1} + t\overrightarrow{u}_{bar2}) - c(\overrightarrow{\omega}_{bar2} \times  tu_{bar2})\times  tu_{bar2}]dt
\end{align} 

\\
\\
acc_{0fixed} = \frac{T}{MI}
}
```

$acc_{0fixed}$ is the angular acceleration that would occur on P0, if the net torque of P1 is zero.

Hence, making angular acceleration equal to $acc_{0fixed}$ on P0 and making P1 seized is equivalent to lending extra torque that would cancel out the net torque on P1.

The next two steps are P1 paying back the extra torque it has borrowed.


#### 2. Calculate Inertia Torque Force on P1

From step 1, bar1 is assumed to make an angular acceleration, and bar1 has its own angular velocity.

The angular acceleration and angular velocity make kinetic force on P1, which results in an inertia force torque on P1.

The angular acceleration on P0 makes plain acceleration on P1 equal to:

```math
acc_{0fixed} \times \overrightarrow{bar1}

```

The angular velocity of P1 referred to as $\omega$ gives centripetal acceleration on P1 equal to:

```math
\omega \times (\omega \times \overrightarrow{bar1})

```

![Payback example 03](./Resources/payback_example_03.jpeg "Payback example 03")


From P2's point of view, bar2 is having acceleration at its center of mass point (this is approximated to the middle point in this implementation) with acceleration equal to:
```math
-(acc_{0fixed} \times \overrightarrow{bar1} + \omega \times (\omega \times \overrightarrow{bar1}))

```

Force is equal to mass times acceleration, so the inertia torque on P1, $T1_i$ is equal to:

```math
T1_i = \frac{\overrightarrow{bar2}}{2} \times -m_2(acc_{0fixed} \times \overrightarrow{bar1} + \omega \times (\omega \times \overrightarrow{bar1}))

```

#### 3. Calculate Acceleration on P1

Calculate the acceleration $acc_{1}$ on P1 that is from the net torque on P1 so far.
Calculate the net torque on P1 so far.
You need to project the torque from the raw forces to the rotational axis, because P1's rotation is only limited to the rotational axis $E_w$.

```math
\displaylines{
    RawT_{bar2} = T1_i +  \frac{|\overrightarrow{bar2}|}{2}(\overrightarrow{W} \times \overrightarrow{bar2}) - \frac{c|\overrightarrow{bar2}|}{3}(\overrightarrow{\omega}_{bar2} \times \overrightarrow{bar2} \times \overrightarrow{bar2}) - k_{p1} \overrightarrow{\Delta\theta}_{bar2}
    \\
    T_{bar2} = (RawT_{bar2} \bullet \overrightarrow{E}_w) \overrightarrow{E}_w 
    \\
    acc_{1} = \frac{T_{bar2}}{MI_{bar2}}
}
```

However, assigning this angular acceleration on P1 requires another payback.

If bar2 was in the middle of space without any pivot, the force applying to bar2 would rotate the bar about its center of mass point.
Making bar2 rotate about P1 due to forces applied to bar2 is equivalent to lending force to P1, which is again equivalent to lending torque to P0.


#### 4. Calculate P1 Payback Torque on P0

First calculate the force F that would give the torque equal to $T_{bar2}$ if it is applied at the opened end of bar2 with stationary pivot P1.

```math
F = T_{bar2} \times \frac{\overrightarrow{bar2}}{2|\overrightarrow{bar2}|}

```

F applied to P1 is an extra force lent to the system in order to make F only create angular acceleration on P1.

![Payback example 04](./Resources/payback_example_04.jpeg "Payback example 04")


The payback torque and angular acceleration $acc0_{payback}$ on P0 is equal to:

```math
\displaylines{
    T = -F \times \overrightarrow{bar1}
    //
    acc_{0payback} = T / MI
}
```


#### 5. Update P0 Angular Acceleration

Calculate the final P0 angular acceleration $acc_{0}$.


```math

acc_{0} = acc_{0fixed} + acc_{0payback}
```

Adding $acc_{0payback}$ again is equivalent to lending extra torque on P1 to cancel the inertia torque caused by the newly added acceleration on P0.

Steps 2 to 5 can be repeated several times with only the newly added acceleration on P0;
However, repeating the payback process is not a convergence guaranteed procedure.

Hence, the current implementation of the study stops at step 5 and does not repeat the payback on P0 and P1.


### Divergence of Infinite Payback Method

Repeating the payback method infinitely on a two-bar linkage system gives two infinite series (one for P0 and the other for P1).

The calculation on these series does not give convergent results for every grass instance.
Depending on the mass and length of the bars, it leads to divergent results.

Let $T0_i$ be torque on P0 and $T1_i$ be torque on P1 from the i'th payback iteration.

Let $T1_1$ be the torque from step 2 of the above example.

Then, $T0_1$ from step 4 is:

```math
\displaylines{
    \begin{align}
        T0_1 & = \overrightarrow{bar1} \times  (T1_1 \times \frac{\overrightarrow{bar2}}{|\overrightarrow{bar2}|^2})
        \\
        & = \frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})T1_1 - (\overrightarrow{bar1} \cdot T1_1)}{|\overrightarrow{bar2}|^2}
        \\
        & = \frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})}{|\overrightarrow{bar2}|^2}T1_1
    \end{align}
}
```

Given $T1_i$ and $T0_i$:

```math
\displaylines{
    T1_{i + 1} = \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2}T1_i

    \\

    T0_{i + 1} = \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2}T0_i
}
```
So both series are geometric series with the same multiplier.

Hence infinitely applying payback would result in:

```math
\displaylines{
    \sum_{i=1}^{\infty}T0_{i + 1} = T0_1/(1 - \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2})
    \\
    \sum_{i=1}^{\infty}T1_{i + 1} = T1_1/(1 - \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2})
}
```
if the multiplier is less than 1.

However, the multiplier $\frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2}$
is not guaranteed to be less than 1 for every grass instance.

When the infinite payback series sums are actually used in the implementation, some grass instances show implausible motions.

### Solving Payback Method as Linear Algebra

An attempt to solve the payback method as linear algebra is made and failed.

Let T0 be the torque applied to P0 due to wind force, air friction force, and restoration force.
Let T1 be the torque applied to P1 in the same manner.

Let K0, K1 be the actual kinetic torque applied to generate the acceleration on P0 and P1 respectively.

Then, the variables form a linear algebra system.

```math
\displaylines{
    T0 - K0 + \frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})}{|\overrightarrow{bar2}|^2}K1 = 0
    \\

    T1 - K1 - \frac{m_2}{2}([\overrightarrow{bar2} \times ((\omega \times(\omega \times (\overrightarrow{bar1})) + \frac{(K0 \times \overrightarrow{bar1})}{MI})] \cdot \overrightarrow{E}_w )\overrightarrow{E}_w = 0
}
```

It seems the payback method can be solved as a linear algebra problem, but the above equation does not guarantee existence of the solution either.

The above linear algebra leads to the solution:

```math
\displaylines{
    K1 = T1 + \frac{m_2}{2}|\omega^2|((\overrightarrow{bar2} \times \overrightarrow{bar1}) \cdot \overrightarrow{E}_w)\overrightarrow{E}_w - \frac{m_2}{2MI}(\overrightarrow{bar1} \cdot \overrightarrow{bar2})(K0 \cdot \overrightarrow{E}_w)\overrightarrow{E}_w
    \\

    K0 \cdot \overrightarrow{E}_w = \frac{
        [T0 \cdot \overrightarrow{E}_w -\frac{\overrightarrow{bar1} \cdot \overrightarrow{bar2}}{|\overrightarrow{bar2}|^2}(T1 + \frac{m_2}{2}|\omega|^2(\overrightarrow{bar2} \times \overrightarrow{bar1}))\cdot \overrightarrow{E}_w]
    }{
        (1 - \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2})
    }
}

```

And this solution faces the same problem as the infinite series solution, which seems to be natural.

It is only solvable when $\frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2}$ is less than 1.


### Implications of the Failures of Convergence of Payback Method

The two previous failures imply that the Payback method is not an unbiased approximation method that can lead to accurate results as the repetition of the payback procedure increases.

Hence, it is only applied once for P0 and P1 in the implementation.

### Angular Displacement Magnitude Limitation on P0

In the real world, grass cannot rotate or twist infinitely. 
A grass blade would have limitations, and it would break if it receives torque beyond its limitation.

Hence, we set a threshold to the magnitude of the angular displacement and make an additional process of keeping the limit.

In dynamics, angular displacement delta $\overrightarrow{\Delta\theta}$ is calculated in each frame and updates the angular displacement.

```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + \overrightarrow{\Delta\theta}

```
$\overrightarrow{\Delta\theta}$ is scaled down if it can lead to threshold breach.

It seems just calculating the length of $d_{old} + \Delta\theta$ is enough to check the threshold breach, but it is not.

In some cases, the final updated angular displacement may not have the threshold breach, but it may have breached the threshold in the middle of the rotation.

Given $\overrightarrow{\Delta\theta}$, its axis of rotation $\overrightarrow{r}$ is 

```math
\overrightarrow{r} = \overrightarrow{\Delta\theta} / |\overrightarrow{\Delta\theta}|
```

Changing the magnitude of $\overrightarrow{\Delta\theta}$ to `t` would result in the new angular displacement becoming

```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t \overrightarrow{r}
```
And the square of the magnitude is equal to 
```math
|\overrightarrow{d}_{new}|^2 = \overrightarrow{d}_{new} \cdot \overrightarrow{d}_{new}  = 
|\overrightarrow{d}_{old}|^2 + t^2 + 2(a \cdot d_{old})
```

Let l be the threshold.


#### Case $|\overrightarrow{d}_{old}| < l$

Let 
```math
    f(t) = (l)^2 - |\overrightarrow{d}_{new}|^2
```

We want to increase t from 0 to the point where $f(t)$ is no longer positive and becomes zero.

Hence solve for 

```math
\displaylines{
   \begin{align}
        0 & = f(t) 
        \\
        0 & = (l)^2 - |\overrightarrow{d}_{old}|^2 - t^2 - 2t(a \cdot \overrightarrow{d}_{old})
    \end{align}
}
```
This is a simple quadratic equation.

Depending on the value of t, different tasks are performed.

1. Only negative real number solutions, or no real number solution for t
    - This case suggests the delta angular displacement axis does not make the angular displacement breach the threshold. 
    - No extra task is done.
2. $|\overrightarrow{\Delta\theta}| <= t$
    - The magnitude of delta angular displacement is not enough to breach the threshold. 
    - No extra task is done.
3. $|\overrightarrow{\Delta\theta}| > t$
    - Set the angular velocity to zero because the bar has reached its threshold and stopped.
        - $$\overrightarrow{\omega}_{new} = \overrightarrow{0}$$
    - Delta angular displacement will make angular displacement breach the threshold. In this case, set the new angular displacement to  

```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t\overrightarrow{a}
```

#### Case $|\overrightarrow{d}_{old}| >= l$

```math
\frac{df}{dt}(t)  = -2t - 2(a \cdot d_{old})
```

We want to increase t from 0 to the point where $\frac{df}{dt}$ begins to stop being positive.

The next task performed is also dependent on the value of t and $(a \cdot d_{old})$.

1. 1. df/dt(0) < 0
    - Set angular velocity to 0.
    - Do not update angular displacement.
2. $|\overrightarrow{\Delta\theta}| <= t$
    - No extra task is done.
3. $|\overrightarrow{\Delta\theta}| > t$
    - Set angular velocity to zero, $$\overrightarrow{\omega}_{new} = \overrightarrow{0}$$
    - Change delta magnitude,
    
```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t \overrightarrow{a}
```

### Angular Displacement Magnitude Limitation on non-ground Pivots

The other pivots are only limited to make rotation about $\overrightarrow{E}_w$. 
So the limitations can be handled more simply.

Make minimum and maximum thresholds so that the bars do not touch each other.
The threshold values are dependent on the static positions of the bars. 

If the delta angle breaches the threshold, adjust the angle to keep the threshold and set the angular velocity to zero.


### Ground Collision
Grass should not rotate below the ground.
Ground collision is approximated with the dot product between the direction vector of the bar connected to P0 and the ground normal vector.

Given threshold value g > 0, ground normal n and let $\overrightarrow{d} = \overrightarrow{bar1}/|\overrightarrow{bar1}|$, 

we want 
```math
\overrightarrow{n} \cdot \overrightarrow{d} > g
```

This would not accurately render the ground collision on steep landscapes. 

![Inaccurate ground collision](./Resources/inaccurate_ground_collision.jpeg "Inaccurate ground collision")

And the collision is only applied to the bar connected to the P0 pivot, so collisions of other bars to the ground are ignored.


The solutions for these problems are not handled in this study, but these problems are noticeable only when the clump of grasses has few blades of grass.

Let $\overrightarrow{r} = \overrightarrow{\Delta\theta} / |\overrightarrow{\Delta\theta}|$, then adding $t\overrightarrow{r}$ to the angular displacement 
yields a new bar direction

```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + sin(t)(r \times d) + (1 - cos(t))(r \times (r \times d))
```


#### Case $\overrightarrow{n} \cdot \overrightarrow{d} > g$

Let
 ``` math
 \displaylines{
     a = (r \times d) \cdot \overrightarrow{n}
    \\
    b = -(r \times (r \times d)) \cdot \overrightarrow{n}
    \\
    c = \overrightarrow{n} \cdot \overrightarrow{d}_{old} - b - g
    \\
    f(t) = \overrightarrow{n} \cdot\overrightarrow{d}_{new} - g
}
 ```


Then,
```math
f(t) = a \sin(t) + b \cos(t) + c
```

We want to increase t from 0 to the point where $f(t)$ is not positive anymore.

Solve t for $f(t) = 0$.

Using trigonometric identity $sin(i + j) = sin(i)cos(j) + cos(i)sin(j)$ to solve the problem. 

Let 
```math
 \displaylines{
    \tan(z) = b/a
    \\
    R = \sqrt{(a/c)^2 + (b/c)^2}
 }
```
then

```math
 \displaylines{
    R \sin(t + z) = 1
    \\
    t = \arcsin(1/R) - z
 }
```

For case $|c|$ is near to zero, solve $f(t) = a \sin(t) + b \cos(t)$. In this case $t = \arctan(-b/a)$

For case $|a|$ is near zero or $|b|$ is near zero, you can solve t with use of $\arccos$ or $\arcsin$.

However, the inverse trigonometric function just gives one value from multiple candidate solutions.
For example $sin(x) = sin(\pi - x)$.

Every possible angular value of t for solving $f(t) = 0$ needs to be checked and the least positive value(after converting the negative angles to positive angles by adding $2\pi$) must be selected.

The next task performed is similar to angular displacement magnitude limitation. Different tasks are performed depending on the value of t.

1. No possible solution for $f(t) = 0$
    - This case suggests the delta angular displacement cannot make the dot value below g.
2. $|\overrightarrow{\Delta\theta}| <= t$
    - The magnitude of delta angular displacement is not enough to breach the threshold. No extra task is done.
3. $|\overrightarrow{\Delta\theta}| > t$
    - Set the angular velocity to zero because the bar has reached its threshold and stopped.
        $$\overrightarrow{\omega}_{new} = \overrightarrow{0}$$
    - Delta angular displacement will make the dot breach the threshold

```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t\overrightarrow{a}
```

#### Case $\overrightarrow{n} \cdot \overrightarrow{d} <= g$

Similar to the angular displacement magnitude limitation, we are interested in $\frac{df}{dt}(t)$

```math
\displaylines{
    a = (r \times d) \cdot \overrightarrow{n}
    \\
    b = (r \times (r \times d)) \cdot \overrightarrow{n}
    \\
    \frac{df}{dt}(t) = a \cos(t) - b \sin(t)
}
```

We want to increase t from 0 to the point where $\frac{df}{dt}$ stops being negative.

If df/dt(0) < 0, then set t = 0; Otherwise solve $\frac{df}{dt}(t) = 0$

For $|a|$ or $|b|$ near to zero, inverse trigonometric functions can be used to solve the problem.

Otherwise,

```math
\displaylines{
    \frac{df}{dt}(t) = 0
    \\
    a \cos(t) - b \sin(t) = 0

    t = \arctan(a/b)
}
```

The next task done with the value of t is: 

1. t = 0
    - Cannot increase the dot value with the current delta axis.
    - Set angular velocity to 0.
    - Do not update angular displacement.
2. $|\overrightarrow{\Delta\theta}| <= t$
    - No extra task is done.
3. $|\overrightarrow{\Delta\theta}| > t$
    - Set angular velocity to zero, $$\overrightarrow{\omega}_{new} = \overrightarrow{0}$$
    - Change delta magnitude,
    
```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t \overrightarrow{a}
```

### Quadratic Bezier Curve Length Control

The length of a Bezier curve changes as the angles between the bars change.
In order to keep constant length on each grass blade instance, each instance needs to be rescaled by the Bezier curve length. 

The reference study does not concern this problem.
The developers of Ghost of Tsushima admit that they do not control the length of the grass blades because it is not noticeable and calculating the length of Bezier curve is a hard problem.
Ghost of Tsushima uses cubic Bezier curve, and indeed, there is no analytical function for length of Bezier curves with degree more than two.
If a Bezier curve has degree equal or greater than 3, it requires numerical method to calculate the length.

However, Quadratic Bezier curve is used in this study, which is degree two Bezier curve, has analytical solution for the curve length.

Given a parametric function $f:\mathbb{R} \to \mathbb{R}^3$, its curve length L is
```math
L = \int_{0}^{x}\left(\sqrt{(\frac{df_x}{dt}(t))^2 + (\frac{df_y}{dt}(t))^2 + (\frac{df_z}{dt}(t))^2}  \right)dt
```
when $t\in [0, x]$

For a quadratic Bezier curve, above equation is simplified to

```math
\displaylines{
    L = \int_{0}^{x}\left(\sqrt{a t^2 + bt + c}  \right)dt
    \\
    \\
    \text{where}
    \\
    a = |\overrightarrow{bar1} - \overrightarrow{bar2}|^2
    \\
    b = 2(\overrightarrow{bar1} \cdot \overrightarrow{bar2} - |\overrightarrow{bar2}|^2)
    \\
    c = |\overrightarrow{bar1}|^2
}
```

Quadratic Bezier curve length can be calculated by integrating the square root of a quadratic equation.

The quadratic discriminant

```math
\begin{align}
4ac - b^2 & = |\overrightarrow{bar1}|^2 |\overrightarrow{bar2}|^2 - 2(\overrightarrow{bar1} \cdot \overrightarrow{bar2})
\\
& = |\overrightarrow{bar1}|^2 |\overrightarrow{bar2}|^2(1 - \cos^2(\theta)))
\\
4ac - b^2 & \ge  0
\end{align}
```

is greater than or equal to zero.


For the case where the quadratic discriminant is equal to zero or below some small threshold, L becomes
```math
L = \int_{0}^{x}(\sqrt{a}\left| t + \frac{b}{2a} \right|)dt
```
which can be solved easily.


For the case where the discriminant is greater than zero or the threshold, there is a general solution that uses trigonometric substitution to solve the integration.
The solution for L is
```math
\displaylines{
    L = \frac{2d}{8a^{3/2}}(k\sqrt{1 + k^2} + \ln(k + \sqrt{1 + k^2}))\Big|_{t=0}^{t=x}
    \\
    \text{where}
    \\
    d = 4ac - b^2
    \\
    k = \frac{2at + b}{d}
}
```
In the implementation of this study $t\in [0, 1]$, making x = 1.

### Wind Generation

In the implementation, the wind force is formed by adding noise to the base wind force.

The noise is generated from several levels of gradient noise function. 
Gradient noise functions with different grid sizes are added to generate noise. 
The noise value is passed to a trigonometric function to generate position-continuous wind noise force.
The noise moves its position according to the direction of the base wind force.

The movement speed of wind noise is linear with the square root of the length of the base wind force.
This is because wind speed is linear with the square root of the wind force magnitude in actual physics.

This is a slight modification of the method in the [presentation made by Pixel Ant Games at GIC](https://youtu.be/LCqeVnmcz3E?si=gny2cCKcLBKCCtnF&t=306)


### Initial Angular Displacement
Grasses are produced at runtime and cleaned up for optimization. 

The motion dynamics is only applied to grasses that generated and not cleaned up.
Hence, the implementation accurately make long time motion that matches with the life time of the grass in the environment.

If the generated grass instance have zero angular displacement, it may have a sudden rise of angular velocity from its zero velocity. 
For this reason, an approximated equilibrium angular displacement with the wind at the current time frame is calculated to minimize the sudden rise of the angular velocity.

The initial angular displacement is made so that the restoration force becomes equal to the wind force at zero angular displacement.
It is only a roughly made approximation because accurate approximation require numerical method.

However, this unnatural motion due to the sudden rise of the angular velocity is not significantly noticeable.
It is ocasinally noticeable when a clump of grasses has small number blades and are generated near to the camera.


### Motion Results

[![Watch the video](https://img.youtube.com/vi/VW9ld_k17Ho/hqdefault.jpg)](https://www.youtube.com/watch?v=VW9ld_k17Ho)

The UE5 implementation provides UI to adjust wind force and air friction coefficient.

It can be observed low air damping coefficient with strong wind force gives somewhat unnatural motion, 
but in real world physics air friction is supposed grow linearly with wind force.

In this implementation, air friction is not automatically adjusted as from the wind force for ease of testing, 
but in the real usecase air friction should grow linearly with wind force.


#### Comparing Results with or without Rotation Limits

[![Watch the video](https://img.youtube.com/vi/gnih4243lIs/hqdefault.jpg)](https://www.youtube.com/watch?v=gnih4243lIs)

Above video uses the methods in this study. 

Bellow video is the result without ground collision and angular magnitude limitation on P0.

Angle limitation on P1 is not removed and still remains because removing it can lead to zero length Bezier curve and yield division by zero error.

[![Watch the video](https://img.youtube.com/vi/3fyWGqc9AHo/hqdefault.jpg)](https://www.youtube.com/watch?v=3fyWGqc9AHo)

It can be observed that, the motion without rotation limits shows unplausible motion when the wind force becomes strong.

It can be observed that too high restoration force of a grass makes chaotic motion that does not make any movement to be aligned with the wind direction.


#### Point Force Results with or without Rotation Limits

Surprisingly, the calculation corrected version of point force(of wind and air friction) motion in the reference study also gives plausible motion 
when the two rotation limiting methods are used.

[![Watch the video](https://img.youtube.com/vi/mJhBVzVHy_0/hqdefault.jpg)](https://www.youtube.com/watch?v=mJhBVzVHy_0)

In above video, 
- Torque is calculated with the assumption that the wind and air friction form a point force, 
  - but corrected version of damping and restoration torque calculation is used.
- Same as the reference study, the feedbacks between bars in bar-linkage system is ignored 
- Following two methods used in the reference study to decrease inconsistent motion are not used.
  - Limiting the rotation of the ground pivot to bending and swinging
  - calculating angular displacement for each group of bars instead of each bars.
- Instead,
    - The ground pivot is rendered as a full 3 dimensional pivot. 
    - Angular displacement is calculated for each bar in two-bar linkage system. 


Compared to the line segment motion with payback method in this study, 
point force motion tend to show more straight form of blades, aligned with the wind direction. 

Also damping force has weaker effect.


Bellow video is the result of using point forces without the rotation limits. The bending limit on P1 is still remained to avoid zero division from zero length bezier curve.

[![Watch the video](https://img.youtube.com/vi/qu_WTiCiIrc/hqdefault.jpg)](https://www.youtube.com/watch?v=qu_WTiCiIrc)

It shows chaotic movement with strong wind force as expected.

## Procedural Grass Generation

Todo
