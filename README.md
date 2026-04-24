# SAR-Framework
A Dynamic Simulation Framework for Mobile Ad Hoc  Networks in Search and Rescue Operations
#  SAR-MANET Simulation Framework (ns-3)

This repository contains the  implementation of a modular Search-and-Rescue (SAR) MANET simulation framework developed in ns-3.

The framework is released to support reproducibility of the results reported in(please cite):

## Scope of this Release
This code corresponds to the framework and includes:
- Core SAR scenario setup
- Communication stack configuration
- Discovery and rescue logic (baseline)
- Logging and dataset generation infrastructure

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

