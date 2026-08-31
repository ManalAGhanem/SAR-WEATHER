# SAR-WEATHER

## A Modular Weather-Aware Simulation Framework for MANET Search-and-Rescue Simulations

SAR-WEATHER is a modular, weather-aware Search-and-Rescue (SAR) MANET simulation framework developed in ns-3.

The framework introduces a shared dynamic environmental state through the `WeatherManager`, allowing weather conditions to influence multiple simulation mechanisms consistently, including wireless propagation, node mobility, line-of-sight (LOS)-based civilian discovery, environmental logging, and mission-level SAR behaviour.

Environmental conditions can be configured individually, combined into compound weather scenarios, or changed dynamically during simulation execution. The framework is intended to support controlled and reproducible evaluation of MANET behaviour under adverse environmental conditions relevant to disaster-response and SAR operations.

The framework supports reproducibility of the experiments reported in:

Ghanem, M., Sabaliauskaite, G., Correia-Hopkins, S., Jones, J. L., & Micallef, N. (2026). A dynamic simulation framework for mobile ad hoc networks in search and rescue operations. Simulation Modelling Practice and Theory, 103281.

---

## Scope of This Release

The repository includes:

- Core heterogeneous SAR scenario setup
- MANET communication-stack configuration
- Civilian discovery and rescue logic
- `WeatherManager` environmental-state module
- Dynamic weather-state scheduling
- Weather-aware propagation modelling
- Rain attenuation
- Fog attenuation
- Atmospheric/gaseous attenuation
- Weather-aware mobility scaling
- Weather-aware LOS adjustment
- Configurable individual and compound weather scenarios
- Configurable mobility-sensitivity analysis
- Environmental and mobility logging
- Seed/run metadata for reproducibility
- Dataset-generation infrastructure

---

# Weather Framework Overview

The framework separates the environmental state from the simulation mechanisms that use it.

A simplified architecture is:

```text
                         WeatherManager
                              |
              +---------------+---------------+
              |               |               |
              v               v               v
      Propagation Model   Mobility Model   LOS / Discovery
              |                               |
              v                               v
       Wireless Channel                 SAR Mission Logic
```

The `WeatherManager` maintains the current environmental conditions and provides a common interface through which other simulation components access the same environmental state.

This allows a weather condition to be defined once and used consistently by the relevant communication, mobility, LOS, and mission-level mechanisms.

---

# Weather Module – Usage Guide

The weather-aware propagation path follows the layered structure:

```text
WeatherManager
      ↓
WeatherAttenuationModel
      ↓
BasePropagationModel
      ↓
Wireless Channel
```

The `WeatherManager` stores and updates environmental parameters.

The `WeatherAttenuationModel` calculates the applicable additional environmental attenuation.

The underlying ns-3 propagation model calculates the baseline propagation loss.

---

## Step 1 — Create the Weather Manager

Create a `WeatherManager` instance:

```cpp
Ptr<WeatherManager> weather = CreateObject<WeatherManager>();
```

---

## Step 2 — Configure Weather Conditions

Environmental conditions are configured using `SetWeatherCondition()`.

```cpp
weather->SetWeatherCondition("RainRate", 30.0);      // Rain rate (mm/h)
weather->SetWeatherCondition("FogDensity", 0.0);     // Fog liquid-water density (g/m^3)
weather->SetWeatherCondition("SnowRate", 0.0);       // Snow rate (mm/h)
weather->SetWeatherCondition("Humidity", 17.0);      // Absolute humidity (g/m^3)
weather->SetWeatherCondition("WetSnow", 0.0);        // 1.0 = treat as wet snow
weather->SetWeatherCondition("WindSpeed", 0.0);      // Wind speed (m/s)
weather->SetWeatherCondition("WindDirection", 0.0);  // Wind direction (degrees)
```

These values can be configured to represent conditions such as:

- Clear weather
- Rain
- Fog
- Snow
- Wind
- Severe weather
- Compound environmental conditions

Not every environmental variable affects every simulation mechanism.

For example:

- rain, fog, humidity, and atmospheric conditions are used by the applicable propagation calculations;
- rain, fog, snow, and wind can influence node mobility;
- rain and fog can modify effective LOS discovery range.

---

## Step 3 — Configure Dynamic Weather Changes

Weather parameters can be changed during simulation execution using standard ns-3 scheduling.

For example:

```cpp
Simulator::Schedule(
    Seconds(50.0),
    &WeatherManager::SetWeatherCondition,
    weather,
    "RainRate",
    100.0
);
```

This changes the rainfall rate to `100 mm/h` at simulation time `t = 50 s`.

More complex time-varying profiles can be constructed by scheduling multiple environmental updates.

---

## Step 4 — Attach Experiment Metadata

Experiment metadata can be stored with the weather logs for reproducibility:

```cpp
weather->SetMetadata(routingsize, seed, runNumber);
```

This provides traceability for:

- routing configuration;
- simulation seed; and
- run number.

---

## Step 5 — Configure Mobility Sensitivity

The strength of weather-driven mobility reduction can be controlled using a global sensitivity parameter:

```cpp
double mobilitySensitivity = 1.0;
weather->SetMobilitySensitivity(mobilitySensitivity);
```

The parameter ranges from `0.0` to `1.0`:

| Value | Behaviour |
|---:|---|
| `0.00` | Weather-driven mobility reduction disabled |
| `0.25` | 25% of the original mobility-reduction strength |
| `0.50` | 50% of the original mobility-reduction strength |
| `0.75` | 75% of the original mobility-reduction strength |
| `1.00` | Full original WeatherManager mobility response |

The original weather-dependent mobility factor is first calculated from the active environmental conditions.

The applied factor is then:

```text
AppliedFactor = 1 - λ × (1 - OriginalFactor)
```

where `λ` is the configured `mobilitySensitivity`.

Therefore:

```cpp
double mobilitySensitivity = 0.0;
```

disables only the **weather-driven mobility slowdown**. Other weather effects, including propagation and LOS effects, remain active.

In contrast:

```cpp
double mobilitySensitivity = 1.0;
```

reproduces the full configured WeatherManager mobility response.

This parameter also enables controlled sensitivity analysis without changing the original node-type-specific weather factors or severity thresholds.

---

## Step 6 — Enable Weather and Mobility Logging

Configure output filenames for environmental-state and mobility logging:

```cpp
weather->SetHistoryFilename(
    routingsize + "_" + std::to_string(runNumber) +
    "_Weather_change_history_finalv3.csv");

weather->SetSpeedLogFile(
    routingsize + "_" + std::to_string(runNumber) +
    "_speed_change_finalv3.csv");
```

Typical generated outputs include:

- Weather change history
- Mobility/speed change log
- Weather state associated with mobility updates
- Original mobility factor
- Applied sensitivity-adjusted mobility factor
- Node speed information
- Experiment metadata

These logs support reproducibility, statistical analysis, and dataset generation.

---

## Step 7 — Create the Baseline Propagation Model

Select an underlying ns-3 propagation model.

Example using the Hybrid Buildings model:

```cpp
Ptr<HybridBuildingsPropagationLossModel> baseloss =
    CreateObject<HybridBuildingsPropagationLossModel>();
```

An alternative model can also be used, for example:

```cpp
// Ptr<FriisPropagationLossModel> baseloss =
//     CreateObject<FriisPropagationLossModel>();
```

The baseline model calculates the standard propagation loss before the additional environmental attenuation is applied.

---

## Step 8 — Create the Weather Attenuation Model

Create the weather-aware propagation wrapper:

```cpp
Ptr<WeatherAttenuationModel> weatherLoss =
    CreateObject<WeatherAttenuationModel>();
```

---

## Step 9 — Connect the Baseline Propagation Model

Attach the baseline propagation model to the weather-aware model:

```cpp
weatherLoss->SetChild(baseloss);
```

Conceptually:

```text
Total Propagation Loss
        =
Baseline Propagation Loss
        +
Applicable Weather Attenuation
```

---

## Step 10 — Connect the Weather Manager

Link the shared environmental state to the attenuation model:

```cpp
weatherLoss->SetWeatherManager(weather);
```

The attenuation model can now access the current weather conditions dynamically during simulation execution.

---

## Step 11 — Configure Propagation Parameters

Configure the parameters required by the atmospheric attenuation calculations:

```cpp
weatherLoss->SetFrequency(2.5);           // GHz
weatherLoss->SetPolarization("horizontal");
weatherLoss->SetTemperature(23.0);        // Celsius
```

---

## Step 12 — Assign the Weather Model to the Channel

Attach the weather-aware propagation model to the wireless channel:

```cpp
channel->SetPropagationLossModel(weatherLoss);
```

The channel is now connected to the weather-aware propagation model.

---

# Part B — Weather-Aware Mobility Adaptation

The `WeatherManager` can dynamically modify node mobility according to the severity of the current environmental conditions.

The mobility mechanism considers:

- Rain
- Fog
- Snow
- Wind

Different mobility-retention factors are defined for different node types, including:

- Foot responders
- Vehicles
- Drones

When multiple environmental stressors are active at the same time, their mobility effects are combined multiplicatively.

Conceptually:

```text
OriginalMobilityFactor =
    RainFactor
    × FogFactor
    × SnowFactor
    × WindFactor
```

Only factors corresponding to active environmental conditions contribute to the resulting mobility factor.

The configured factors represent controlled simulation stress parameters and can be evaluated using the `mobilitySensitivity` parameter described above.

---

## Step 13 — Schedule Mobility Evaluation

Example for foot responder nodes:

```cpp
for (uint32_t i = 0; i < footNodes.GetN(); i++)
{
    Ptr<Node> node = footNodes.Get(i);
    Ptr<MobilityModel> mob =
        node->GetObject<MobilityModel>();

    Simulator::Schedule(
        Seconds(1.0),
        &WeatherManager::ScheduleMobilityReduction,
        weather,
        mob,
        "foot",
        1.0
    );
}
```

### Parameter Meaning

- `Seconds(1.0)` — schedules the first mobility evaluation at `t = 1 s`
- `weather` — the `WeatherManager` instance
- `mob` — the node's mobility model
- `"foot"` — node mobility category
- final `1.0` — mobility reevaluation interval in seconds

The final `1.0` is a **time interval**, not a mobility-reduction factor.

With this configuration, mobility is evaluated approximately every second so that changes in the environmental state can be reflected dynamically.

---

## How Weather-Aware Mobility Works

During each mobility evaluation, the `WeatherManager`:

1. Reads the current rain, fog, snow, and wind conditions.
2. Determines the corresponding weather-severity states.
3. Selects the node-type-specific mobility factors.
4. Combines the applicable environmental factors.
5. Applies the configured `mobilitySensitivity`.
6. Updates the node's mobility model.
7. Records the resulting mobility state in the speed log.

This supports scenarios such as:

- Reduced responder movement under severe rainfall
- Reduced vehicle mobility under adverse environmental conditions
- Reduced movement under snow
- Wind-related mobility constraints
- Compound environmental effects
- Controlled sensitivity analysis of mobility assumptions

---

# Part C — Weather-Aware LOS Adjustment

The shared environmental state can also be used by the SAR discovery mechanism to modify effective line-of-sight range.

The implemented LOS adaptation considers:

- rainfall; and
- fog.

Weather-related LOS scaling is applied independently of building-based geometric occlusion.

This allows the framework to distinguish between:

```text
Physical/Building Occlusion
```

and:

```text
Weather-Related Visibility Reduction
```

during civilian discovery.

The LOS scaling equations are intended as configurable simulation couplings for controlled environmental stress testing rather than empirically calibrated field-visibility predictors.

---

# Example Compound Weather Configuration

A compound severe-weather configuration can be defined as:

```cpp
weather = CreateObject<WeatherManager>();

weather->SetWeatherCondition("RainRate", 20.0);
weather->SetWeatherCondition("FogDensity", 0.8);
weather->SetWeatherCondition("SnowRate", 30.0);
weather->SetWeatherCondition("Humidity", 17.0);
weather->SetWeatherCondition("WetSnow", 1.0);
weather->SetWeatherCondition("WindSpeed", 25.0);
weather->SetWeatherCondition("WindDirection", 0.0);

weather->SetMetadata(routingsize, seed, runNumber);

double mobilitySensitivity = 1.0;
weather->SetMobilitySensitivity(mobilitySensitivity);
```

Individual weather scenarios can be created by setting the unused environmental variables to zero.

---

# Example Mobility-Sensitivity Experiment

The environmental configuration can be kept constant while changing only:

```cpp
double mobilitySensitivity = 0.0;
```

or:

```cpp
double mobilitySensitivity = 0.25;
```

or:

```cpp
double mobilitySensitivity = 0.50;
```

or:

```cpp
double mobilitySensitivity = 0.75;
```

or:

```cpp
double mobilitySensitivity = 1.0;
```

This allows the effect of the assumed mobility impairment to be evaluated independently of changes in the underlying environmental conditions.

---

# Requirements

- ns-3 version: **ns-3.43**
- Compiler: **g++ with C++17 support**
- Tested on: **Linux (Ubuntu 22.04)**

---

# How to Build

```bash
./ns3 configure
./ns3 build
```

---

# Running the Experiments

The experiments reported in the paper can be executed using the provided bash script to maintain consistent protocol, random-seed, and run-number configuration.

## Option A — Scripted Execution

```bash
chmod +x simrun.sh
./simrun.sh
```

## Option B — Manual Single Run

```bash
./ns3 run "scratch/V2.cc" -- --routing=AODV --scenario=V2 --RngRun=1
```

Scenario names and command-line parameters should be adjusted to match the supplied simulation configuration.
