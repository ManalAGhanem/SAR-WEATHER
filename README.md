# SAR-WEATHER

A Dynamic Weather-Aware Simulation Framework for Mobile Ad Hoc Networks in Search and Rescue Operations  

# SAR-MANET Weather-Aware Simulation Framework (ns-3)

This repository contains the implementation of a **modular, weather-aware Search-and-Rescue (SAR) MANET simulation framework** developed in ns-3.

The framework extends the baseline SAR-MANET architecture by incorporating environmental and weather effects into wireless communication modeling, enabling realistic performance evaluation under harsh disaster conditions.

The framework is released to support reproducibility of the results reported in (please cite):



## Scope of this Release

This code corresponds to the weather module implemenation:

- Core SAR scenario setup  
- Communication stack configuration  
- Discovery and rescue logic (baseline)  
- Logging and dataset generation infrastructure  
- Weather-aware propagation modeling  
- Environmental attenuation integration into PHY layer  
- Configurable weather scenarios for experimentation  


## Weather Model Overview

The Weather Module introduces environmental awareness into the wireless channel model.

Unlike conventional MANET simulations that assume ideal conditions, this framework incorporates:

- Rain-induced signal attenuation  
- Fog-based absorption and scattering  
- Humidity-related propagation degradation  
- Severe weather composite effects  
#  Weather Module – Complete Usage Guide

The Weather Module integrates environmental conditions into the wireless propagation model using a layered architecture:

WeatherManager → WeatherAttenuationModel → BasePropagationModel → Channel

The WeatherManager stores environmental parameters.  
The WeatherAttenuationModel computes additional signal attenuation.  
The BasePropagationModel computes standard distance-based loss.  

Follow the steps below to enable weather in your simulation.

---

## Step 1 — Create the Weather Manager

The WeatherManager controls all environmental parameters and logging.

```cpp
Ptr<WeatherManager> weather = CreateObject<WeatherManager>();
```

---

## Step 2 — Configure Weather Conditions

Set environmental parameters using `SetWeatherCondition()`.

```cpp
weather->SetWeatherCondition("RainRate", 30.0);     // Rain intensity (mm/hr)
weather->SetWeatherCondition("FogDensity", 0.0);    // Fog density (normalized)
weather->SetWeatherCondition("SnowRate", 0.0);      // Snow rate
weather->SetWeatherCondition("Humidity", 17.0);     // Relative humidity (%)
weather->SetWeatherCondition("WetSnow", 0.0);       // 1.0 = treat as wet snow
weather->SetWeatherCondition("WindSpeed", 0.0);     // Wind speed
weather->SetWeatherCondition("WindDirection", 0.0); // Wind direction (degrees)
```

You can modify these values to simulate:

- Clear weather
- Light rain
- Heavy rain
- Fog
- Snow
- Storm conditions

and you can add time-bound chnages:
```cpp
Simulator::Schedule(Seconds(50.0), &WeatherManager::SetWeatherCondition, weather, "RainRate", 100.0);
```
---

## Step 3 — Attach Experiment Metadata (Reproducibility)

This allows traceability of routing configuration, seed, and run number.

```cpp
weather->SetMetadata(routingsize, seed, runNumber);
```

---

## Step 4 — Enable Weather Logging (Dataset Generation)

Configure filenames for logging environmental and mobility changes.

```cpp
weather->SetHistoryFilename(
    routingsize + "_" + std::to_string(runNumber) +
    "_Weather_change_history_finalv3.csv");

weather->SetSpeedLogFile(
    routingsize + "_" + std::to_string(runNumber) +
    "_speed_change_finalv3.csv");
```

Generated files:

- Weather change history (CSV)
- Speed change log (CSV)

These logs are useful for statistical analysis and ML dataset creation.

---

## Step 5 — Create the Baseline Propagation Model

Select a standard ns-3 propagation model.

Example using Hybrid Buildings:

```cpp
Ptr<HybridBuildingsPropagationLossModel> baseloss =
    CreateObject<HybridBuildingsPropagationLossModel>();
```

Alternative:

```cpp
// Ptr<FriisPropagationLossModel> baseloss = CreateObject<FriisPropagationLossModel>();
```

This model calculates the baseline path loss without environmental effects.

---

## Step 6 — Create the Weather Attenuation Model

Create the weather-aware propagation wrapper.

```cpp
Ptr<WeatherAttenuationModel> weatherLoss =
    CreateObject<WeatherAttenuationModel>();
```

---

## Step 7 — Stack the Baseline Model Inside the Weather Model

Attach the baseline propagation model as a child of the weather model.

```cpp
weatherLoss->SetChild(baseloss);
```

Conceptually:

TotalLoss = BasePropagationLoss + WeatherAttenuation

---

## Step 8 — Connect Weather Manager to Propagation Model

Link the environmental parameters to the attenuation model.

```cpp
weatherLoss->SetWeatherManager(weather);
```

This enables dynamic reading of rain, fog, humidity, and other parameters.

---

## Step 9 — Configure Physical Layer Parameters

Set frequency, polarization, and temperature.

```cpp
weatherLoss->SetFrequency(2.5);          // GHz
weatherLoss->SetPolarization("horizontal");
weatherLoss->SetTemperature(23.0);       // Celsius
```

These parameters influence atmospheric absorption calculations.

---

## Step 10 — Assign Weather Model to the Channel

Attach the weather-aware propagation model to the channel.

```cpp
channel->SetPropagationLossModel(weatherLoss);
```

The simulation is now weather-aware.


---

# Part B — Enabling Weather-Based Speed Reduction (Mobility Adaptation)

In addition to signal attenuation, the WeatherManager can dynamically reduce node mobility speed based on environmental severity.

This is achieved using `ScheduleMobilityReduction()`.

---

## Step 11 — Schedule Mobility Reduction

Example for foot-based responder nodes:

```cpp
for (uint32_t i = 0; i < footNodes.GetN(); i++)
{
    Ptr<Node> node = footNodes.Get(i);
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();

    Simulator::Schedule(
        Seconds(2.0),
        &WeatherManager::ScheduleMobilityReduction,
        weather,
        mob,
        "foot",
        2.0
    );
}
```

---

## Explanation of Parameters

- `Seconds(2.0)` → Time when mobility reduction is applied  
- `weather` → WeatherManager instance  
- `mob` → Node’s MobilityModel  
- `"foot"` → Mobility category (e.g., pedestrian responder)  
- `2.0` → Reduction factor or severity parameter  

---

## How Speed Reduction Works

When scheduled:

1. WeatherManager reads current environmental conditions  
2. Determines severity (rain, snow, wind, etc.)  
3. Applies a reduction factor to the node’s mobility model  
4. Logs the speed change in the speed log file  

This allows simulation of:

- Slower responders in heavy rain  
- Reduced movement in snow  
- Mobility constraints in severe storm conditions  

---


## Requirements
- ns-3 version: **ns-3.43**
- Compiler: g++ (C++17)
- Tested on: Linux (Ubuntu 22.04)

## How to Build

./ns3 configure
./ns3 build

## How to run

## Running the Experiments

The experiments reported in the paper were executed using the provided bash script to ensure consistent protocol selection and random seed control.

### Option A: Recommended (scripted execution)

chmod +x simrun.sh
./simrun.sh


### Option B: Manual execution (single run)
./ns3 run "scratch/V2.cc" -- --routing=AODV --scenario=V2 --RngRun=1
