# Dockerfile para el Nodo Satélite Integrado (SuchaiFS Sim + HoneySat)

# 1. Etapa de construcción (Build Stage)
FROM ubuntu:22.04 as builder

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential cmake git python3 python3-pip pkg-config \
    libzmq3-dev curl wget

# Copiamos la versión local de Suchai-Sim (que ya tiene los parches de CMake)
COPY ./src/suchai-sim /opt/suchai-sim
WORKDIR /opt/suchai-sim

# Aseguramos que suchai-flight-software y sus submódulos estén presentes
# (Si ya están en la carpeta local, esto será rápido)
RUN if [ ! -d "suchai-flight-software" ]; then sh init.sh; fi
WORKDIR /opt/suchai-sim/suchai-flight-software
RUN git submodule update --init --recursive

# Aplicamos los parches necesarios para Python 3 en el core de SuchaiFS
RUN wget -O src/lib/libcsp/waf https://waf.io/waf-2.1.1 && chmod +x src/lib/libcsp/waf
RUN sed -i 's/python2/python3/g' src/lib/CMakeLists.txt
RUN sed -i "s/'0x0000'/0/g" src/lib/libcsp/wscript

# Compilamos la aplicación PLANTSAT
WORKDIR /opt/suchai-sim
RUN mkdir -p build && cd build && \
    cmake .. -DAPP=plantsat -DSCH_NAME=PLANTSAT && \
    cmake --build .

# 2. Etapa final (Final Stage)
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    python3 python3-pip python3-zmq libzmq5 \
    iproute2 iputils-ping net-tools telnet \
    && rm -rf /var/lib/apt/lists/*

# Copiamos el binario de SuchaiFS (Plantsat)
COPY --from=builder /opt/suchai-sim/build/apps/plantsat/suchai-app /usr/local/bin/suchai-app
RUN ldconfig

# Copiamos el simulador de HoneySat
COPY ./src/honeysat/deployment/projects/honeysat-api /opt/honeysat-api
WORKDIR /opt/honeysat-api

# Instalamos dependencias en bloques para evitar timeouts
RUN pip3 install --no-cache-dir numpy==1.26.4 scipy==1.15.2 pandas==2.2.3 matplotlib==3.10.1
RUN pip3 install --no-cache-dir casadi==3.6.7 jax==0.4.27 jaxlib==0.4.27
RUN pip3 install --no-cache-dir pybamm==25.1.1 pybammsolvers==0.1.0 skyfield==1.52
RUN pip3 install --no-cache-dir -r requirements.txt

# Script de inicio
RUN echo '#!/bin/bash\n\
export PYTHONPATH=/opt/honeysat-api\n\
# Iniciar simulador físico en segundo plano\n\
python3 -u TestsAndExamples/TestZMQInterface.py &\n\
# Dar tiempo al simulador para arrancar\n\
sleep 5\n\
# Iniciar Software de Vuelo\n\
exec suchai-app\n\
' > /usr/local/bin/entrypoint.sh && chmod +x /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
