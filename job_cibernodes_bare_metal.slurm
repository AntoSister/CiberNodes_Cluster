#!/bin/bash
#SBATCH --job-name=CiberNodes_BM_Sim
#SBATCH --partition=batch
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=2
#SBATCH --mem=4G
#SBATCH --time=00:30:00
#SBATCH --output=sim_bm_%j.out
#SBATCH --error=sim_bm_%j.err

PROJECT_ROOT=$PWD
export PYTHONPATH=$PROJECT_ROOT/src/honeysat/deployment/projects/honeysat-api
export PATH=$HOME/.local/bin:$PATH
export LD_LIBRARY_PATH=$PROJECT_ROOT/src/suchai-sim/build/src/lib/libcsp:$HOME/.local/lib:$LD_LIBRARY_PATH

# Variables requeridas por HoneySat
export SATELLITE_NAME_TLE="SUCHAI-2"
export SATELLITE_NORAD_CATALOG_NUMBER="52194"
export GROUND_STATION_LAT="-33.4569"
export GROUND_STATION_LON="-70.6661"
export TLE_FILE="$PROJECT_ROOT/src/honeysat/deployment/projects/honeysat-api/TLE_and_data/suchai-2.tle"
export DE421_PATH="$PROJECT_ROOT/data_efem/de421.bsp"

echo "--- INICIANDO SIMULACIÓN BARE METAL ---"

# 1. Iniciar simulador físico (HoneySat)
# Usamos la ruta de efemérides descargada
python3 -u $PROJECT_ROOT/src/honeysat/deployment/projects/honeysat-api/TestsAndExamples/TestZMQInterface.py > honeysat_phys.log 2>&1 &
PHYS_PID=$!

sleep 5

# 2. Iniciar Software de Vuelo (SuchaiFS)
$PROJECT_ROOT/src/suchai-sim/build/apps/plantsat/suchai-app

# Al terminar SuchaiFS, limpiamos el simulador
kill $PHYS_PID
echo "--- SIMULACIÓN FINALIZADA ---"
