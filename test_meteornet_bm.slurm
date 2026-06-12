#!/bin/bash
#SBATCH --job-name=MeteorNet_BM_Test
#SBATCH --partition=batch
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=2G
#SBATCH --time=00:05:00
#SBATCH --output=test_bm_%j.out
#SBATCH --error=test_bm_%j.err

# --- EXPLICACIÓN DEL SCRIPT ---
# 1. Entramos a la carpeta de MeteorNet porque el código espera rutas relativas (tles, data, etc.)
# 2. Ejecutamos MeteorNet con el nuevo flag --bare_metal que creamos.
# 3. Solo pedimos 1 satélite y 20 segundos para que sea una prueba rápida.

echo "--- INICIANDO PRUEBA DE METEORNET EN MODO BARE METAL ---"
cd $HOME/CiberNodes_Cluster/src/meteornet

# Aseguramos que la carpeta de logs exista y esté limpia para la prueba
rm -f sat_network.log

# Ejecución de MeteorNet
# --bare_metal: Activa nuestra lógica sin contenedores
# --sats 1: Solo un satélite para validar la orquestación
# --sim_seconds 20: Una simulación corta
python3 main.py --bare_metal --sats 1 --sim_seconds 20

echo ""
echo "--- VERIFICACIÓN DE RESULTADOS ---"
if grep -q "MODO BARE METAL" sat_network.log; then
    echo "¡ÉXITO! Se detectó la ejecución en modo Bare Metal en los logs."
    echo "Mensajes encontrados:"
    grep "MODO BARE METAL" sat_network.log
else
    echo "ATENCIÓN: No se encontró el mensaje de Bare Metal. Revisa test_bm_$SLURM_JOB_ID.err"
fi

echo ""
echo "--- PRUEBA FINALIZADA ---"
