# SAR-WEATHER

A Dynamic Weather-Aware Simulation Framework for Mobile Ad Hoc Networks in Search and Rescue Operations  

# SAR-MANET Weather-Aware Simulation Framework (ns-3)

This repository contains the implementation of a **modular, weather-aware Search-and-Rescue (SAR) MANET simulation framework** developed in ns-3.

The framework extends the baseline SAR-MANET architecture (https://github.com/ManalAGhanem/SAR-Framework.git) by incorporating environmental and weather effects into wireless communication modeling, enabling realistic performance evaluation under harsh disaster conditions.

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
