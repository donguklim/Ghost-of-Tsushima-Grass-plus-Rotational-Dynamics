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

Note: The study uses the term "edge" to refer the bars in the bar-linkage rotational system, but in physics studies that involves a rotational system "bar" is a term more often used. 
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

The pivot at ground can only roatate about the land normal and $\overrightarrow{E}_w$ vector direction of the grass blase(shown in the above image).

The rotation about land normal is referred as swinging and the rotation about $\overrightarrow{E}_w$ is referred as bending.

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

#### Feedbacks between bars in Bar-Linkage System

If there are multiple bars connected with rotational pivots, a force acting on a bar may not just influence torque on the base pivot of the bar. 
Bellow image shows two example with two-bar linkage system.

![Linkage System Force Examples](./Resources/linkage_system_force_examples.jpeg "Linkage System Force Examples")

As shown in above image,  a force actiong on a point of a bar may create torque with opposite or the same orientation to another bar 
depending on the angle between the bars and the force direction.

A force acting on a bar is not guranteed to make a torque on the single pivot at the base of the bar. 

There are feedbacks between bars.

The reference study omits this physical consideration and calculate each pivot torque independently with each bar. 


## Methods Used in This Study.

The baisc idea of using rotational system dynamics is same as the reference study.

Limitation of the rotation to bending on non-ground pivots and sampling single wind force samping per grass instance from the reference study are also used.

Limitation of rotation to bending is used for more plausible motion. 
Single wind sampling per grass instance is used for faster computation.


However, the torque calculation is differs from the refernce study as wind and air friction forces are applied at line segments 
and an approximation method is used for reflecting physical characteristics of the bar-linkage systems.

Also, there are additional methods for preventing distorted motionss.

The methods in this study is implemented with two-bars linkage system. Quadratic Bezier curve is used to render the grasses in the implementation.


### Payback Torque Calculation Method

A calculation method which would be referred as 'payback method' is devised in this study to account the feedbacks between bars.

The method will be explaned with two-bar linkage system example.

A two-bar linkage system with bar1, bar2, stationary pivot P0 and another pivot p1 that connects the bars are given.


![Payback example 01](./Resources/payback_example_01.jpeg "Payback example 01")

#### 1. Calculate P1 seized P0 Angular Acceleration

Calculate the angular acceleration $acc_{0fixed}$ that would occur at P0 if P1 did not exist and bar1 and bar2 have fixed connection.

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

Hence, making angular acceleration equal to $acc_{0fixed}$ on P0 and make P1 seized is equivalent to lending extra torque that would cancel out the net torque on P1.

Next two steps are P1 paying back the extra torque it has burrowed. 


#### 2. Calculate intertia torque force on P1.

From step1, bar 1 is assumed to make an angular acceleration, and bar1 has its own angular velocity. 

The angular acceleration and angular velocity make kinetic force on P1, which results an intertia force torque on P1.

The angular acceleration on P0, makes plain acceleration on P1 equal to

```math
acc_{0fixed} \times \overrightarrow{bar1}

```

The angualr velocity of P1 refered as $\omega$, gives centripetal acceleration on P1 equal to 

```math
\omega \times (\omega \times \overrightarrow{bar1})

```

![Payback example 03](./Resources/payback_example_03.jpeg "Payback example 03")


At P2's point of view, bar2 is having acceleration at its center of mass point(this is approximated to the middle point in this implementation) with acceleration equal to
```math
-(acc_{0fixed} \times \overrightarrow{bar1} + \omega \times (\omega \times \overrightarrow{bar1}))

```

Force is equal to mass times acceleration, so the inertia torque on P1, $T1_i$ is equal to 

```math
T1_i = \frac{\overrightarrow{bar2}}{2} \times -m_2(acc_{0fixed} \times \overrightarrow{bar1} + \omega \times (\omega \times \overrightarrow{bar1}))

```

#### 3. Calculate acceleration on P1

Calculate the acceleration $acc_{1}$ on P1 that is from the net torque on P1 so far.
Calculate the net torque on P1 so far.
You need to project the torque from the raw forces to the rotational axis, because P1's rotation is only limited to the rotational axis $E_w$

```math
\displaylines{
    RawT_{bar2} = T1_i +  \frac{|\overrightarrow{bar2}|}{2}(\overrightarrow{W} \times \overrightarrow{bar2}) - \frac{c|\overrightarrow{bar2}|}{3}(\overrightarrow{\omega}_{bar2} \times \overrightarrow{bar2} \times \overrightarrow{bar2}) - k_{p1} \overrightarrow{\Delta\theta}_{bar2}
    \\
    T_{bar2} = (RawT_{bar2} \bullet \overrightarrow{E}_w) \overrightarrow{E}_w 
    \\
    acc_{1} = \frac{T_{bar2}}{MI_{bar2}}
}
```

However assigning this angular acceleration on P1 requires another payback. 

If bar2 was in the middle of space without any pivot, the force applying to bar2 would rotate the bar about its center of mass point.
Making bar2 to rotate about P1 due to forces applied to bar2 is equivalent to lending force to P1, which is again equivalent to lending torque to P0.


#### 4. Calculate P1 payback Torque on P0

First calculate the force F that would give the torque equal to $T_{bar2}$ if it is applied at the opend end of bar2 with stationary pivot p1.

```math
F = T_{bar2} \times \frac{\overrightarrow{bar2}}{2|\overrightarrow{bar2}|}

```

F applied to P1 is an extra force lent to the system in order to make F to only make angular acceleration on P1. 

![Payback example 04](./Resources/payback_example_04.jpeg "Payback example 04")


The payback torque and angualr acceleration $acc0_{payback}$ on P0 is equal to 

```math
\displaylines{
    T = -F \times \overrightarrow{bar1}
    //
    acc_{0payback} = T / MI
}
```


#### 5. Update P0 Angular acceleration

Calculate the final P0 angular acceleration $acc_{0}$.


```math

acc_{0} = acc_{0fixed} + acc_{0payback}
```

Adding $acc_{0payback}$ again is equivalent to lending extra torque on P1 to cancel the inertia torque caused by the newly added acceleration on P0.

Step 2 to 5 can be repeated several times with only the newly added acceleration on P0; 
However, repeating the payback process is not a convergence guranteed procedure.

Hence, current implementation of the study stops at step 5 and does not repeat the payback on p0 and p2.


### Divergence of Infinite Payback Method

Repeating the payback method infinitely on two-bar linkage system gives two infinite series(one for P1 and the other for P1).

The calculation on these series doses not give convergent result to every grass instance. 
Depending on the mass and length of the bars, it leads to divergent result.

Let $T0_i$ be torque on P0 and $T1_i$ be torque on P1 from i'th payback iteration.

Let $T1_1$ be the torque from step 2 of above example.

Then, $T0_1$ from step 4 is

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

Given $T1_i$ and $T0_i$,

```math
\displaylines{
    T1_{i + 1} = \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2}T1_i

    \\

    T0_{i + 1} = \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2}T0_i
}
```
So both series are geometric sereis with the same multipler. 

Hence infinitely applying payback would result

```math
\displaylines{
    \sum_{i=1}^{\infty}T0_{i + 1} = T0_1/(1 - \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2})
    \\
    \sum_{i=1}^{\infty}T1_{i + 1} = T1_1/(1 - \frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2})
}
```
if the multiplier is less than 1.

However, the multipler $\frac{m_2}{2MI}\frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})^2}{|\overrightarrow{bar2}|^2}$ 
is not guranteed to be less than 1 for every grass instance.

When the infinite payback is actaully used in the implementation, 
some grass instances show plausible motions, but some instances shows unnatural motions or disappears due to error.

### Solving Payback method as Linear Algebra

Attempt to solve the payback method as linear algebra is made and failed.

Let T0 be the torque applied to P0 due to wind force, air friction force and restoration force.
Let T1 be the torque applied to P1 in the same manner.

Let K0, K1 be the actual kinetic torque applied to generate the acceleration on P0 and P1 respectively.

Then, the variables form a linear algebra.

```math
\displaylines{
    T0 - K0 + \frac{(\overrightarrow{bar1} \cdot \overrightarrow{bar2})}{|\overrightarrow{bar2}|^2}K1 = 0
    \\

    T1 - K1 - \frac{m_2}{2}([\overrightarrow{bar2} \times ((\omega \times(\omega \times (\overrightarrow{bar1})) + \frac{(K0 \times \overrightarrow{bar1})}{MI})] \cdot \overrightarrow{E}_w )\overrightarrow{E}_w = 0
}
```

It seems the payback method can be solved as a linear algebra problem, but above equation does not gurantee existence of the solution either.

Above linear algebra leads to the solution 

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


### Implications of the Failures of convergence of Payback Method

The two previous failures imply that Payback method is not an unbiased approximation method that can lead to accurate result as the repeatance of the payback procedure increases.

Hence, it is only applied once for P0 and P1 in the implementation.

### Angular Displacement Magnitude Limitation on P0

In the real world, a grass cannot rotate or twist infinitely. 
A grass blade would have limitation, and it would be broken if it receives torque beyond its limitation.

Hence, we set threshold to the magnitude of the angular displacement and make additional process of keeping the limit.

In dynamics, angular displacement delta $\overrightarrow{\Delta\theta}$ is calculated in each frame and updates the angualr displacement.

```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + \overrightarrow{\Delta\theta}

```
$\overrightarrow{\Delta\theta}$ is scaled down if it can lead to threshold breach.

It seems just calculating the length of $d_{old} + \Delta\theta$ is enough to check the threashold breach, but it is not.

In some cases, the final updated angular displacement may not have the threshold breach, but it may have breached the threshold in the middle of the rotation.

Given $\overrightarrow{\Delta\theta}$, its axis of rotation `a` is 

```math
\overrightarrow{a} = \overrightarrow{\Delta\theta} / |\overrightarrow{\Delta\theta}|
```

Changing the magnitude of $\overrightarrow{\Delta\theta}$ would result the new angualr displacement to become

```math
\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t \overrightarrow{a}
```
And the square of the magnitude is equal to 
```math
|\overrightarrow{d}_{new}|^2 = \overrightarrow{d}_{new} \cdot \overrightarrow{d}_{new}  = 
|\overrightarrow{d}_{old}|^2 + t^2 + 2(a \cdot d_{old})
```

Let l be the threshold.


#### Case $|\overrightarrow{d}_{old}| < l$

We want to solve t that would make the magnitude equal to l.

So solve t for, 

```math
\displaylines{
    |\overrightarrow{d}_{new}|^2 - (l)^2 = 0 
    \\
    |\overrightarrow{d}_{old}|^2 + t^2 + 2t(a \cdot \overrightarrow{d}_{old}) - (l)^2 = 0
}
```
This is a simple quadratic equation.

Depending the value of t, different task is performed.

1. No real number solution for t
    - this case suggests the delta axis does not make the angular displacement to breach the threshold. No extra task is done.
2. $|\overrightarrow{\Delta\theta}| <= t$
    - the magnitude of delta is not enough to breach the threshold. No extra task is done.
3. $|\overrightarrow{\Delta\theta}| > t$
    - delta will make angular displacement to breach threshold in this case set the new angular displacement to 
      - $\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t\overrightarrow{a}$
    - and set the angular velocity to zero because the bar has reached to its threshold and stopped.
        $$\overrightarrow{\omega}_{new} = \overrightarrow{0}$$


#### Case $|\overrightarrow{d}_{old}| >= l$

Let f be the function of the magnitude square of the new angular displacement with the delta magnitude t.

```math
f(t)  = |\overrightarrow{d}_{old}|^2 + t^2 + 2t(a \cdot d_{old})
```
Then, 

```math
df/dt(t)  = 2t + 2(a \cdot d_{old})
```
And we want to find value of twhere df/dt stops being positive ans is greater or equal to 0.

If df/dt(0) < 0, then set t = 0; Other wise $t = -(a \cdot d_{old})$


The angular displacement magnitude will be decreasing upto this t value. 
The next task perfromed is also dependent to the value of t.


1. t = 0
    - cannot decrease the angular displacement magnitude with current delta axis.
    - set angualr velocity to 0
    - do not update angular displacement.
2. $|\overrightarrow{\Delta\theta}| <= t$
    - No extra task is done.
3. $|\overrightarrow{\Delta\theta}| > t$
    - change delta magnitude, $$\overrightarrow{d}_{new} = \overrightarrow{d}_{old} + t \overrightarrow{a}$$
    - set angular velocity to zero, $$\overrightarrow{\omega}_{new} = \overrightarrow{0}$$


### Angular Displacement Magnitude Limitation on non-ground Pivots

The other pivots are only limited to make rotation about $\overrightarrow{E}_w$. 
So the limitations can be handled more simply.

Make minimum and maximum thersholds so that the bars do not touch each other.
The threshold values are dependent to the static positions of the bars. 

If the delta angle breaches the threshold, adjust the angle to keep threshold and set the angular velocity to zero.


### Ground Collision
Grass should not rotate bellow the ground.
Ground collision is approximated with the dot product between the direction vector of the bar connected to P0 and the ground normal vector.

Given threshold value g > 0, ground normal n and let $\overrightarrow{d} = \overrightarrow{bar1}/|\overrightarrow{bar1}|$, 

we want 
```math
\overrightarrow{n} \cdot \overrightarrow{d} > g
```

This would not accurately render the ground collision on steep landscapes. 

![Inaccurate ground collision](./Resources/inaccurate_ground_collision.jpeg "Inaccurate ground collision")

And the collision is only applied to the bar connected to the P0 pivot, so collisions of other bars to the ground are ignored.


The solutions for these problems are not handled in this study, but these problems are noticable only when the clump of grasses have few blades of grasses.


#### Case $\overrightarrow{n} \cdot \overrightarrow{d} > g$