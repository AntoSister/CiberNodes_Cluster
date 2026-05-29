#!/bin/bash
#SBATCH --job-name=CiberNodes_Sim
#SBATCH --partition=batch
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=2G
#SBATCH --output=cibernodes-%j.out
#SBATCH --error=cibernodes-%j.err

echo "=========================================================="
echo "   EJECUTANDO CIBERNODES EN XI CLUSTER (USACH)            "
echo "=========================================================="

IMAGE="acisternas/cibernodes:latest"

# 2. Ejecutamos SIN entrypoint automático para evitar bloqueos de namespace
srun --container-image="$IMAGE" \
     --container-mounts="/home/ceph/acisternas/CiberNodes_Cluster:/opt/workspace" \
     --no-container-entrypoint \
     /bin/bash /usr/local/bin/entrypoint.sh
