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

# 1. Imagen en Docker Hub (Cámbialo por tu usuario de Docker Hub si es distinto)
# Si tu usuario es 'antosister', quedará así:
IMAGE="antosister/cibernodes:latest"

# 2. Ejecutamos usando la ruta de CEPH que nos dio tu 'pwd'
srun --container-image="$IMAGE" \
     --container-mounts="/home/ceph/acisternas/CiberNodes_Cluster:/opt/workspace" \
     /usr/local/bin/entrypoint.sh
