#!/bin/bash
#SBATCH --job-name=CiberNodes_Setup
#SBATCH --partition=batch
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=8G
#SBATCH --time=01:00:00
#SBATCH --output=setup_cibernodes-%j.out
#SBATCH --error=setup_cibernodes-%j.err

echo "--- INICIANDO PREPARACIÓN DE ENTORNO CIBERNODES EN XI CLUSTER ---"

# Flujo: Obtener (ubuntu:22.04) -> Montar ($PWD) -> Guardar (cibernodes_env)
srun --container-image="ubuntu:22.04" \
     --container-mounts="$PWD:/workspace" \
     --container-save="cibernodes_env" \
     /bin/bash /workspace/setup_cluster_env.sh

echo "--- PROCESO FINALIZADO. IMAGEN 'cibernodes_env' CREADA ---"
