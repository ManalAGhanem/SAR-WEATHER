set -euo pipefail

PROGRAM="scratch/V1.cc"
SCENARIO="V3"

ROUTING_LIST=(  "OLSR" "AODV" "DSDV" )         # add "DSDV" "OLSR" etc if needed
RUN_START=1
RUN_END=10 # change to 10 for 1..10

export NS3_SCENARIO="$SCENARIO"

# Optional: ensure ns-3 helper exists
command -v ./ns3 >/dev/null || { echo "Could not find ./ns3 in this directory"; exit 1; }

for proto in "${ROUTING_LIST[@]}"; do
  for ((i=RUN_START; i<=RUN_END; i++)); do
    echo "Running $proto with RngRun=$i"
    ./ns3 run "$PROGRAM" --  --routing="$proto" --scenario="$SCENARIO" --RngRun="$i" 
  done
done

