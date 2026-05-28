import logging
from datetime import datetime

# Configuración básica de logging para consola
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

class Logger:
    @staticmethod
    def write_data(_class_name, _id, _data, _log_type):
        # Simplemente imprimimos en el log de Python en lugar de usar MongoDB
        log_msg = f"[{_class_name}] ID:{_id} DATA:{_data}"
        if _log_type.lower() == 'info':
            logging.info(log_msg)
        elif _log_type.lower() == 'error':
            logging.error(log_msg)
        elif _log_type.lower() == 'warning':
            logging.warning(log_msg)
        else:
            logging.debug(log_msg)
