from ryu.app.simple_switch_stp_13 import SimpleSwitch13
from ryu.lib import dpid as dpid_lib


class STPSwitch(SimpleSwitch13):
    def __init__(self, *args, **kwargs):
        super(STPSwitch, self).__init__(*args, **kwargs)
        config = {i: {'bridge': {'hello_time': 1, 'fwd_delay': 2}} for i in range(80)}
        self.stp.set_config(config)


