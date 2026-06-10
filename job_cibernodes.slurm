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

# Ejecutamos la imagen local creada con el script de preparación
srun --container-image="cibernodes_env" /usr/local/bin/entrypoint.sh
