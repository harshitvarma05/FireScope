# CPU reference model

This implements the simplified model proposed in PROJECT_SPEC.md. It is not
calibrated to operational data. All updates read only the current timestep;
state, fuel, and burn-age arrays swap after the whole next grid is written.

## Coordinates and spread

- +x is east, +y is north. The flat viewer puts north at the top.
- Eight Moore neighbors; no wraparound at edges.
- Relative wind angle is measured from the burning source to the target.
- Cardinal separation is cellSizeMeters; diagonal separation is sqrt(2) times it.
- Slope is atan((target elevation - source elevation) / separation).

For each burning neighbor, the probability is:

```
p = clamp(baseProbability * (0.5 + vegetation) * (0.5 + density)
          * exp(Kwind * windSpeed
                + Kalignment * windSpeed * (cos(relativeAngle) - 1)
                + Kslope * slope - Kmoisture * effectiveMoisture), 0, 1)
effectiveMoisture = 1 - (1 - cellMoisture) * (1 - globalHumidity)
Ptotal = 1 - product(1 - p)
```

The 0.5–1.5 vegetation/density multipliers are explicit project choices. The
exponent is bounded to [-80, 80] for numerical stability. Constants live in
EnvironmentParams. Neighbor loops always use the same order.

## Randomness and fuel

The stateless unsigned 32-bit hash uses seed, x, y, and timestep. Arithmetic wraps
modulo 2^32. Ignition compares the hash with floor(Ptotal * 2^32), using uint64_t
for the threshold so probability 1 ignites even for the largest hash. Float
probability evaluation may still differ between future CPU/CUDA/Metal backends;
integer random thresholds alone do not guarantee exact backend agreement.

Burning cells consume fuel each step. They become Burned only when fuel is zero
and minimumBurnTicks has elapsed. NoFuel and Burned cells are absorbing states during simulation updates.
Water increases moisture and does not directly extinguish existing fire.

The backend owns elapsed timesteps. EnvironmentParams.timestep is reserved for
the future GPU dispatch parameters; CPU hashes use its own counter. Reset restores
grid data and timestep zero. The application reapplies the same central ignition.

## Correctness coverage

The tests cover fixed PRNG vectors, probability thresholds (including 0 and 1),
no ignition, eight-neighbor ignition, synchronous propagation, corner boundaries,
a full firebreak, fuel exhaustion, minimum burn duration, deterministic reset,
batched steps, cell counts, and paired-seed wind/slope/moisture experiments.
Paired one-step trials isolate those factors without assuming an outcome for
every stochastic multi-step scenario.


## Vegetation editing

The Vegetation brush assigns the chosen value (clamped to 0–1) to fuel,
vegetation and density for NoFuel or Unburned cells. Positive values create
Unburned forest; zero sets NoFuel. Burn age resets to zero. Existing moisture
and elevation are retained. Burning and Burned cells are preserved by this tool;
Firebreak remains the explicit tool for clearing an active fire. NoFuel remains
absorbing during CA steps, but an explicit vegetation edit can regrow it.

Brushes do not advance the logical timestep. The same edit rule exists in CPU,
CUDA and Metal; CPU/Metal behavior has been verified, CUDA remains pending
NVIDIA validation. Snapshots capture these edited properties for reproducible
paired comparisons. Water adds the requested amount per stamp and saturates at
one; no moisture decay is implemented.
