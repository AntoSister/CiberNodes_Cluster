from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI
from mininet.node import RemoteController, OVSController


class MyTreeTopo( Topo ):
    """
        Custom definition of TreeTopo where protocols of switch is OpenFlow13.
        It is copied from TreeTopo class with slight change.
    """
    "Topology for a tree network with a given depth and fanout."

    def build( self, depth=1, fanout=2 ):
        # Numbering:  h1..N, s1..M
        self.hostNum = 1
        self.switchNum = 1
        # Build topology
        self.addTree( depth, fanout )

    def addTree( self, depth, fanout ):
        """Add a subtree starting with node n.
           returns: last node added"""
        isSwitch = depth > 0
        if isSwitch:
            node = self.addSwitch( 's%s' % self.switchNum, protocols="OpenFlow13" )  # This line has been modified
            self.switchNum += 1
            for _ in range( fanout ):
                child = self.addTree( depth - 1, fanout )
                self.addLink( node, child )
        else:
            node = self.addHost( 'h%s' % self.hostNum )
            self.hostNum += 1
        return node


if __name__ == "__main__":
    #creating topology and connecting to contoller
    # controller = RemoteController('c0', ip='127.0.0.1', port=6653)
    controller = OVSController("c0")
    topo = MyTreeTopo(depth=2, fanout=4)
    net = Mininet(topo=topo, controller=controller)
    net.start()

    #IF you want to start the CLI
    CLI(net)
    #If you want to clean the environment
    net.stop()