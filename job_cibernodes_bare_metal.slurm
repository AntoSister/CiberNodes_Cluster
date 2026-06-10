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
export LD_LIBRARY_PATH=$PROJECT_ROOT/src/suchai-sim/build/src/lib/libcsp:$LD_LIBRARY_PATH

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
