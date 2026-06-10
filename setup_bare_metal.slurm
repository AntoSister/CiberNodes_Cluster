#!/bin/bash
#SBATCH --job-name=CiberNodes_BM_Setup
#SBATCH --partition=batch
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=8G
#SBATCH --time=01:00:00
#SBATCH --output=setup_bm_%j.out
#SBATCH --error=setup_bm_%j.err

set -e

PROJECT_ROOT=$PWD
export PATH=$HOME/.local/bin:$PATH

echo "--- INICIANDO PREPARACIÓN BARE METAL EN XI CLUSTER ---"

echo "1. Instalando dependencias de Python (Local)..."
# Usamos --user para no necesitar root
pip3 install --user --upgrade pip
pip3 install --user numpy==1.26.4 scipy==1.15.2 pandas==2.2.3 matplotlib==3.10.1
pip3 install --user casadi==3.6.7 jax==0.4.27 jaxlib==0.4.27
pip3 install --user pybamm==25.1.1 pybammsolvers==0.1.0 skyfield==1.52
pip3 install --user -r src/honeysat/deployment/projects/honeysat-api/requirements.txt

echo "2. Preparando SuchaiFS y parches..."
cd src/suchai-sim/suchai-flight-software
if [ ! -d "src/lib/libcsp" ]; then sh ../init.sh; fi
wget -O src/lib/libcsp/waf https://waf.io/waf-2.1.1 && chmod +x src/lib/libcsp/waf
sed -i 's/python2/python3/g' src/lib/CMakeLists.txt
sed -i "s/'0x0000'/0/g" src/lib/libcsp/wscript

echo "3. Compilando SuchaiFS (Plantsat SIM)..."
cd $PROJECT_ROOT/src/suchai-sim
rm -rf build && mkdir build && cd build
cmake .. \
    -DAPP=plantsat \
    -DSCH_OS=LINUX \
    -DSCH_ARCH=SIM \
    -DSCH_NAME=PLANTSAT \
    -DSCH_LOG=INFO \
    -DSCH_ST_MODE=SQLITE \
    -DSCH_COMM_NODE=1 \
    -DSCH_DEVICE_ID=2 \
    -DSCH_CSP_BUFFERS=100 \
    -DSCH_SIM_INTERFACE_URI="tcp://localhost:5555"
make -j4

echo "4. Descargando efemérides (de421.bsp)..."
mkdir -p $PROJECT_ROOT/data_efem
python3 -c "from skyfield.api import Loader; load = Loader('$PROJECT_ROOT/data_efem'); load('de421.bsp')"

echo "--- PREPARACIÓN BARE METAL COMPLETADA ---"
echo "Binario disponible en: $PROJECT_ROOT/src/suchai-sim/build/apps/plantsat/suchai-app"
