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
# Solo PATH es seguro exportar globalmente
export PATH=$HOME/.local/bin:$PATH

echo "--- INICIANDO PREPARACIÓN BARE METAL EN XI CLUSTER ---"

echo "1. Instalando libzmq localmente..."
if [ ! -f "$HOME/.local/include/zmq.h" ]; then
    mkdir -p $PROJECT_ROOT/deps && cd $PROJECT_ROOT/deps
    wget https://github.com/zeromq/libzmq/releases/download/v4.3.5/zeromq-4.3.5.tar.gz
    tar -xzf zeromq-4.3.5.tar.gz
    cd zeromq-4.3.5
    ./configure --prefix=$HOME/.local --without-libsodium --disable-perf
    make -j4 install
    cd $PROJECT_ROOT
else
    echo "libzmq ya está instalado en ~/.local"
fi

echo "2. Instalando dependencias de Python (Local)..."
pip3 install --user --upgrade pip
pip3 install --user numpy==1.26.4 scipy==1.15.2 pandas==2.2.3 matplotlib==3.10.1
pip3 install --user casadi==3.6.7 jax==0.4.27 jaxlib==0.4.27
pip3 install --user pybamm==25.1.1 pybammsolvers==0.1.0 skyfield==1.52
pip3 install --user -r src/honeysat/deployment/projects/honeysat-api/requirements.txt

echo "3. Preparando SuchaiFS y parches..."
cd src/suchai-sim/suchai-flight-software
# Asegurar que init.sh se ejecute desde la carpeta correcta
if [ ! -d "src/lib/libcsp" ]; then 
    cd ..
    sh init.sh
    cd suchai-flight-software
fi
wget -O src/lib/libcsp/waf https://waf.io/waf-2.1.1 && chmod +x src/lib/libcsp/waf
sed -i 's/python2/python3/g' src/lib/CMakeLists.txt
sed -i "s/'0x0000'/0/g" src/lib/libcsp/wscript

echo "4. Compilando SuchaiFS (Plantsat SIM)..."
cd $PROJECT_ROOT/src/suchai-sim
rm -rf build && mkdir build && cd build
# Pasamos las rutas locales directamente a CMake
cmake .. \
    -DCMAKE_PREFIX_PATH=$HOME/.local \
    -DCMAKE_INCLUDE_PATH=$HOME/.local/include \
    -DCMAKE_LIBRARY_PATH=$HOME/.local/lib \
    -DAPP=plantsat \
    -DSCH_OS=LINUX \
    -DSCH_ARCH=SIM \
    -DSCH_NAME=PLANTSAT \
    -DSCH_LOG=INFO \
    -DSCH_ST_MODE=SQLITE \
    -DSCH_COMM_NODE=1 \
    -DSCH_DEVICE_ID=2 \
    -DSCH_CSP_BUFFERS=100 \
    -DSCH_SIM_INTERFACE_URI="tcp://localhost:5555" \
    -DSCH_SW_VERSION="3.0.0-cibernodes" \
    -DCMAKE_C_FLAGS="-w -Wno-error -I$HOME/.local/include" \
    -DCMAKE_EXE_LINKER_FLAGS="-L$HOME/.local/lib"
make -j4

echo "5. Descargando efemérides (de421.bsp)..."
mkdir -p $PROJECT_ROOT/data_efem
python3 -c "from skyfield.api import Loader; load = Loader('$PROJECT_ROOT/data_efem'); load('de421.bsp')"

echo "--- PREPARACIÓN BARE METAL COMPLETADA ---"
