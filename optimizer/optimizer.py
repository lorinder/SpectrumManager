#!/usr/bin/env python3

import collector
import subprocess

class ChannelProblemInstance(object):
    def __init__(self):
        pass

    def collect_data(self, conn, cursor, channels):
        self.nodes = collector.get_nodes(conn, cursor)
        self.channels = channels

        self.graph = collector.get_graph(conn, cursor, self.nodes)

        # channel scores
        self.chan_scores = []
        for i in range(len(self.nodes)):
            sc_this_if = collector.get_node_channel_scores(conn,
                        cursor,
                        self.nodes,
                        i,
                        self.graph.get(i, []),
                        channels)
            self.chan_scores.append(sc_this_if)

        # channel TX values
        self.tx_values = []
        for i in range(len(self.nodes)):
            tx = collector.get_tx_value(conn, cursor, self.nodes, i)
            self.tx_values.append(tx)

    def get_interface_ids(self):
        """Return the radio_if_id values of all the nodes."""
        for (ifid, _) in self.nodes:
            yield ifid

    def evaluate_assignment(self, assign):
        assert len(assign) == len(self.nodes)
        score = 0

        # First add the baseline scores
        for (i, v) in enumerate(assign):
            score += self.chan_scores[i][v]

        # Then add the interference components
        for i in range(len(self.nodes)):
            neigh = self.graph.get(i, [])
            tx = self.tx_values[i]
            for j in neigh:
                if assign[i] != assign[j]:
                    continue
                score += tx

        return score

    def find_best_assignment(self, verbose=True):
        """Find the best possible assignment (exhaustive search)

        Note that this is an exponential algorithm, and is practical
        only for tiny instances.  For larger networks, a better method
        is needed.
        """
        score_assign_pairs = []
        for ass in _cartesian(len(self.nodes), len(self.channels)):
            v = self.evaluate_assignment(ass)
            score_assign_pairs.append( (v, ass) )
        score_assign_pairs.sort()
        if verbose:
            print("Best assignments, by score (lower is better):")
            for sc, ass in score_assign_pairs[0:10]:
                print("  %.2g  %r" % (sc, ass))
        return score_assign_pairs[0][1]

def _cartesian(n, k):
    """Cartesian product.

    Generator producing the all the length n vectors
    with values [0,...k - 1].
    """
    if n == 0:
        yield []
        return
    for i in range(k):
        for ass0 in _cartesian(n - 1, k):
            yield [ i ] + ass0

def _usage():
    print("Wifi spectrum optimizer")
    print("")
    print("   -h          display this help and exit")
    print("   -d <dbname> name of the DB [wifispecman]")
    print("   -c <ch>     comma separated list of channels to consider")
    print("   -s          perform channel assignments after finding")
    print("               good solution")
    print("   -r #        set collector history size to given value")

def main():
    # Default settings
    dbname = 'wifispecman'
    channels = [ 5180, 5220, 5240 ]
    do_set = False

    # Read options
    import sys
    import getopt
    opts, args = getopt.getopt(sys.argv[1:], 'hd:c:sr:')
    for k, v in opts:
        if k == '-h':
            _usage()
            sys.exit(0)
        elif k == '-d':
            dname = v
        elif k == '-c':
            channels = [ int(x) for x in v.split(",") ]
        elif k == '-s':
            do_set = True
        elif k == '-r':
            collector.hist_size = float(v)

    # Open DB
    conn, cursor = collector.open_db(dbname)

    # Get values for instance
    inst = ChannelProblemInstance()
    inst.collect_data(conn, cursor, channels)

    # Evaluate the different assignments,
    best_ass = inst.find_best_assignment()
    print("The best assignment is", best_ass)

    # Apply the settings to the nodes
    if do_set:
        ip_ifnames = collector.get_ip_ifname_for_ifaces(
                        conn, cursor, inst.get_interface_ids())
        for i, (ip, ifname) in enumerate(ip_ifnames):
            cmd = [ "ssh", "-n", "-T", "root@%s" % (ip,),
                    "./setchan.sh", ifname, str(channels[best_ass[i]]) ]
            print("Running", " ".join(cmd))
            result = subprocess.run(cmd,
                            stdin=subprocess.DEVNULL,
                            timeout=25)
            print("--> Got exitcode", result.returncode)

    # Finish
    conn.close()

if __name__ == "__main__":
    main()

# vim:sts=4:et
