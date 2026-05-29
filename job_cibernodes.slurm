#!/bin/bash
#SBATCH --job-name=CiberNodes_Sim
#SBATCH --partition=batch
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=2G
#SBATCH --output=cibernodes-%j.out
#SBATCH --error=cibernodes-%j.err

echo "--- PRUEBA MINIMALISTA ---"

# Ejecutamos la imagen tal cual, sin montajes ni nada extra
srun --container-image="acisternas/cibernodes:latest" /usr/local/bin/entrypoint.sh
