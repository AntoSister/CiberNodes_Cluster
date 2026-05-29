import time
import zmq
import random
import struct
from PIL import Image

SIM_OBC_ID = 0X01
SIM_OBC_ADDR_TEMP = 0X00

SIM_EPS_ID = 0X02
SIM_EPS_ADDR_HKP = 0X00
SIM_EPS_ADDR_SET = 0X01
SIM_EPS_ADDR_HEATER = 0X02
SIM_EPS_ADDR_HARD_RESET = 0X03

SIM_ADCS_ID = 0X03
SIM_ADCS_ADDR_MAG = 0X00
SIM_ADCS_ADDR_GYR = 0X01
SIM_ADCS_ADDR_SUN = 0X02
SIM_ADCS_ADDR_MTT = 0X03

SIM_CAM_ID = 0x04
SIM_CAM_ADDR_SIZE = 0x00
SIM_CAM_ADDR_TAKE = 0x01
SIM_CAM_PATH_LEN = 256

cam_size_x = 1024
cam_size_y = 1024


def process_obc_request(message: bytes) -> bytes:
    if message[1] == SIM_OBC_ADDR_TEMP:
        temp = random.randint(-40, 125)
        reply = struct.pack('i', temp)
        print(f"OBC temp = {temp} ºC")
    else:
        reply = message
    return reply


def process_eps_request(message: bytes) -> bytes:
    if message[1] == SIM_EPS_ADDR_HKP:
        vbat = random.randint(0, 8200)  # mV
        current_in = random.randint(0, 6000)  # mA
        current_out = random.randint(0, 12000)  # mA
        temp = random.randint(-40, 125)  # ºC
        reply = struct.pack('iiii', vbat, current_in, current_out, temp)
        print(f"EPS HK: VBat={vbat}, CurrIn={current_in}, CurrOut={current_out}, Temp={temp}")
    elif message[1] == SIM_EPS_ADDR_SET:
        reply = struct.pack('i', 1)  # Send status ok
    elif message[1] == SIM_EPS_ADDR_HEATER:
        reply = struct.pack('i', 1)  # Send status ok
    elif message[1] == SIM_EPS_ADDR_HARD_RESET:
        reply = struct.pack('i', 1)  # Send status ok
    else:
        reply = message
    return reply


def process_adcs_request(message: bytes) -> bytes:
    if message[1] == SIM_ADCS_ADDR_MAG:
        x, y, z = [random.random()*8000-4000 for i in range(3)]
        reply = struct.pack('fff', x, y, z)
        print(f"Mag: {x:.02f} {y:.02f} {z:.02f} mG")
    elif message[1] == SIM_ADCS_ADDR_GYR:
        x, y, z = [random.random() * 450 - 225 for i in range(3)]
        reply = struct.pack('fff', x, y, z)
        print(f"Mag: {x:.02f} {y:.02f} {z:.02f} mG")
    elif message[1] == SIM_ADCS_ADDR_SUN:
        sun = random.randint(0, 930)
        reply = struct.pack('i', sun)
        print(f"Sun[{message[2]}] = {sun}")
    else:
        reply = message
    return reply


def process_cam_request(message: bytes) -> bytes:
    if message[1] == SIM_CAM_ADDR_SIZE:
        global cam_size_x, cam_size_y
        cam_size_x, cam_size_y = struct.unpack("ii", message[2:])
        reply = struct.pack("i", 1)
    elif message[1] == SIM_CAM_ADDR_TAKE:
        img = Image.new("RGB", (cam_size_x, cam_size_y))
        img_path = "/tmp/img_{}.png".format(int(time.time()))
        img.save(img_path)
        img_path = img_path.encode('ascii')
        img_path += b"\0"*(SIM_CAM_PATH_LEN-len(img_path))
        reply = img_path
    else:
        reply = message

    return reply


def process_request(message: bytes) -> bytes:
    if message[0] == SIM_OBC_ID:
        reply = process_obc_request(message)
    elif message[0] == SIM_EPS_ID:
        reply = process_eps_request(message)
    elif message[0] == SIM_ADCS_ID:
        reply = process_adcs_request(message)
    elif message[0] == SIM_CAM_ID:
        reply = process_cam_request(message)
    else:
        reply = message
    return reply


def server() -> None:
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.bind("tcp://*:5555")

    try:
        while True:
            request = socket.recv()
            print(f"Received request: {request}")
            reply = process_request(request)
            print(f"Sending reply: {reply}")
            socket.send(reply)
    except Exception as e:
        print(e)
    finally:
        socket.close()
        context.destroy()


if __name__ == '__main__':
    server()
