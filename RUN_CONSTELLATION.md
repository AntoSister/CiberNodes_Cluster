# Guía de Ejecución Rápida: Constelación CiberNodes

Sigue estos pasos en orden para garantizar una simulación limpia y exitosa en el Xi Cluster.

## Paso 1: Limpieza Total (Borrar el pasado)
Ejecuta este bloque para detener procesos huérfanos y limpiar logs viejos:
```bash
pkill -f honeysat; pkill -f suchai-app; pkill -f TestZMQInterface; rm -f ~/CiberNodes_Cluster/*.log ~/CiberNodes_Cluster/test_bm_*.out ~/CiberNodes_Cluster/test_bm_*.err
```

## Paso 2: Sincronizar Código (Asegurar el presente)
Asegúrate de tener la última versión del orquestador (`vPro_PythonEnv`):
```bash
cd ~/CiberNodes_Cluster
git fetch origin main
git reset --hard origin/main
```

## Paso 3: Lanzar Constelación (Iniciar el futuro)
Lanzamos 3 satélites por 10 minutos (600 segundos) en modo Bare Metal:
```bash
cd ~/CiberNodes_Cluster/src/meteornet
python3 main.py --bare_metal --sats 3 --sim_seconds 600
```

## Paso 4: Monitoreo (Ver el presente)
Abre otra terminal y usa estos comandos para ver qué está pasando:

- **Ver logs de física (Telemetría):**
  ```bash
  tail -f ~/CiberNodes_Cluster/honeysat_st1.log
  ```
- **Ver logs de software de vuelo:**
  ```bash
  tail -f ~/CiberNodes_Cluster/suchaifs_st1.log
  ```
- **Verificar que los 3 satélites estén vivos:**
  ```bash
  ls -lh ~/CiberNodes_Cluster/honeysat_st*.log
  ```

---
*Nota: Los satélites usan puertos dinámicos empezando desde el 5569 (st1), 5571 (st2), 5573 (st3).*
