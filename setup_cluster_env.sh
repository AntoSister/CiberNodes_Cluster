#!/bin/bash
# setup_cluster_env.sh - Script de instalación interna para Xi Cluster
set -e

echo "1. Instalando dependencias del sistema..."
apt-get update && apt-get install -y \
    build-essential cmake git python3 python3-pip pkg-config \
    libzmq3-dev curl wget python3-zmq libzmq5 \
    iproute2 iputils-ping net-tools telnet \
    hping3 nmap netcat-openbsd

echo "2. Aplicando parches a libcsp y preparando SuchaiFS..."
cd /workspace/src/suchai-sim/suchai-flight-software
if [ ! -d "src/lib/libcsp" ]; then sh ../init.sh; fi
wget -O src/lib/libcsp/waf https://waf.io/waf-2.1.1 && chmod +x src/lib/libcsp/waf
sed -i 's/python2/python3/g' src/lib/CMakeLists.txt
sed -i "s/'0x0000'/0/g" src/lib/libcsp/wscript

echo "3. Compilando SuchaiFS (Plantsat SIM)..."
cd /workspace/src/suchai-sim
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

echo "4. Instalando binarios y librerías en el sistema..."
cp /workspace/src/suchai-sim/build/apps/plantsat/suchai-app /usr/local/bin/
find /workspace/src/suchai-sim/build -name "libcsp.so*" -exec cp {} /usr/local/lib/ \;
ldconfig

echo "5. Instalando HoneySat y sus dependencias (esto tomará tiempo)..."
mkdir -p /opt/honeysat-api
cp -r /workspace/src/honeysat/deployment/projects/honeysat-api/* /opt/honeysat-api/
cd /opt/honeysat-api
pip3 install --no-cache-dir numpy==1.26.4 scipy==1.15.2 pandas==2.2.3 matplotlib==3.10.1
pip3 install --no-cache-dir casadi==3.6.7 jax==0.4.27 jaxlib==0.4.27
pip3 install --no-cache-dir pybamm==25.1.1 pybammsolvers==0.1.0 skyfield==1.52
pip3 install --no-cache-dir -r requirements.txt

echo "6. Descargando efemérides (de421.bsp)..."
mkdir -p /opt/honeysat-api/TLE_and_data
python3 -c "from skyfield.api import Loader; load = Loader('/opt/honeysat-api/TLE_and_data'); load('de421.bsp')"

echo "7. Creando script de entrada (entrypoint)..."
echo '#!/bin/bash
export PYTHONPATH=/opt/honeysat-api
# Iniciar simulador físico en segundo plano
python3 -u /opt/honeysat-api/TestsAndExamples/TestZMQInterface.py > /var/log/honeysat.log 2>&1 &
# Dar tiempo al simulador para arrancar
sleep 5
# Iniciar Software de Vuelo
exec /usr/local/bin/suchai-app
' > /usr/local/bin/entrypoint.sh
chmod +x /usr/local/bin/entrypoint.sh

echo "--- CONFIGURACIÓN DE ENTORNO COMPLETADA ---"
