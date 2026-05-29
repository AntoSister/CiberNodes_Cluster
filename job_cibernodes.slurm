#!/bin/bash
#SBATCH --job-name=CiberNodes_Sim
#SBATCH --partition=batch
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=2
#SBATCH --mem=4G
#SBATCH --output=cibernodes-%j.out
#SBATCH --error=cibernodes-%j.err

echo "=========================================================="
echo "   EJECUTANDO CIBERNODES EN XI CLUSTER (USACH)            "
echo "=========================================================="

# 1. Imagen sin el prefijo manual, ya que el sistema parece añadirlo solo
IMAGE="ghcr.io/antosister/cibernodes_cluster:latest"

# 2. Ejecutamos usando la ruta de CEPH
srun --container-image="$IMAGE" \
     --container-mounts="/home/ceph/acisternas/CiberNodes_Cluster:/opt/workspace" \
     /usr/local/bin/entrypoint.sh
