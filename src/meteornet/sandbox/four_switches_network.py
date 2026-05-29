from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSController
from mininet.node import CPULimitedHost, Host, Node
from mininet.node import OVSKernelSwitch, UserSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.nodelib import LinuxBridge
from mininet.node import Ryu
from mininet.link import TCLink, Intf
from _thread import start_new_thread
import os, stat
import json
import time
import csv
import requests
import sys

def four_switches_network():
    net = Mininet(ipBase='10.0.0.0/8')

    info('*** Adding controller\n')
    c0 = RemoteController('c0')
    net.addController(c0)
    info('*** Add switches\n')
    s1 = net.addSwitch('s1')
    s2 = net.addSwitch('s2')
    s3 = net.addSwitch('s3')
    s4 = net.addSwitch('s4')

    s5 = net.addSwitch('s5')

    info('*** Add hosts\n')
    h1 = net.addHost('h1')
    h2 = net.addHost('h2')
    h3 = net.addHost('h3')

    h4 = net.addHost('h4')
    h5 = net.addHost('h5')
    h6 = net.addHost('h6')

    h7 = net.addHost('h7')

    info('*** Add links\n')

    net.addLink(h1, s1)
    net.addLink(h2, s1)
    net.addLink(h3, s1)

    net.addLink(h4, s3)
    net.addLink(h5, s3)
    net.addLink(h6, s3)
    net.addLink(h7, s5)

    net.addLink(s1, s2)
    net.addLink(s2, s3)
    net.addLink(s1, s4)
    net.addLink(s4, s3)

    net.addLink(s3, s5)

    info('*** Starting network\n')
    net.start()
    # info('*** Starting controllers\n')
    # c0.start()
    #
    # info('*** Starting switches\n')
    # net.get('s1').start([c0])
    # net.get('s2').start([c0])
    # net.get('s3').start([c0])
    # net.get('s4').start([c0])

    CLI(net)
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')

    four_switches_network()