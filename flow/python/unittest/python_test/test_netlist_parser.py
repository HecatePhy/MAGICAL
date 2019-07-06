##
# @file test_netlist_parser.py
# @author Keren Zhu
# @date 06/28/2019
# @brief Unittest for netlist parsers
#

import os 
import sys
import glob
import unittest
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
import DesignDB as pyDB
import magicalFlow

class TestNetlistParser(unittest.TestCase):
    def setUp(self):
        self.spectre_comparator = os.path.dirname(os.path.abspath(__file__)) + "/../test_data/spectre_netlists/Comparator.sp"
        self.hspice_buff = os.path.dirname(os.path.abspath(__file__)) + "/../test_data/hspice_netlists/BUFF.sp"
    def runTest(self):
        self.spectre_parser_comparator_test()
        self.hspice_parser_buff_test()
    def hspice_parser_buff_test(self):
        db = magicalFlow.DesignDB()
        nlp = pyDB.Netlist_parser(db)
        nlp.parse_hspice(self.hspice_buff)
        self.assertEqual(db.numCkts(), 4)
        db.findRootCkt()
        topckt = db.subCkt(db.rootCktIdx())
        self.assertEqual(topckt.name, "BUFF")
    def spectre_parser_comparator_test(self):
        db = magicalFlow.DesignDB()
        nlp = pyDB.Netlist_parser(db)
        nlp.parse_spectre(self.spectre_comparator)
        self.assertEqual(db.numCkts(), 18)
        """
        for ckt_idx in range(db.numCkts()):
            ckt = db.subCkt(ckt_idx)
            print("Ckt ", ckt.name)
            print("# of nodes ", ckt.numNodes())
            for node_idx in range(ckt.numNodes()):
                node = ckt.node(node_idx)
                print("Node ", node.name, " subgraph index ", node.graphIdx)
        """
        db.findRootCkt()
        topckt = db.subCkt(db.rootCktIdx())
        self.assertEqual(db.rootCktIdx(), 0) # topcircuit is node 0. others are CMOS
        for node_idx in range(topckt.numNodes()):
            node = topckt.node(node_idx)
            if node.name == "M0":
                self.check_cmos_net(0, node_idx, db, "GND", "INTERN", "GND", "GND")
            elif node.name == "M22":
                self.check_cmos_net(0, node_idx, db, "GND", "INTERP", "GND", "GND")
            elif node.name == "M16":
                self.check_cmos_net(0, node_idx, db, "OUTM", "CROSSP", "GND", "GND")
            elif node.name == "M17":
                self.check_cmos_net(0, node_idx, db, "OUTP", "CROSSN", "GND", "GND")
            elif node.name == "M4":
                self.check_cmos_net(0, node_idx, db, "CROSSN", "CROSSP", "INTERN", "GND")
            elif node.name == "M3":
                self.check_cmos_net(0, node_idx, db, "CROSSP", "CROSSN", "INTERP", "GND")
            elif node.name == "M7":
                self.check_cmos_net(0, node_idx, db, "net050", "CLK", "GND", "GND")
            elif node.name == "M5":
                self.check_cmos_net(0, node_idx, db, "INTERN", "VI+", "net050", "GND")
            elif node.name == "M6":
                self.check_cmos_net(0, node_idx, db, "INTERP", "VI-", "net050", "GND")
            elif node.name == "M8":
                self.check_cmos_net(0, node_idx, db, "OUTM", "CROSSP", "VDD", "VDD")
            elif node.name == "M18":
                self.check_cmos_net(0, node_idx, db, "INTERN", "CLK", "VDD", "VDD")
            elif node.name == "M15":
                self.check_cmos_net(0, node_idx, db, "OUTP", "CROSSN", "VDD", "VDD")
            elif node.name == "M19":
                self.check_cmos_net(0, node_idx, db, "INTERP", "CLK", "VDD", "VDD")
            elif node.name == "M10":
                self.check_cmos_net(0, node_idx, db, "CROSSN", "CLK", "VDD", "VDD")
            elif node.name == "M12":
                self.check_cmos_net(0, node_idx, db, "CROSSP", "CLK", "VDD", "VDD")
            elif node.name == "M14":
                self.check_cmos_net(0, node_idx, db, "CROSSN", "CROSSP", "VDD", "VDD")
            elif node.name == "M13":
                self.check_cmos_net(0, node_idx, db, "CROSSP", "CROSSN", "VDD", "VDD")

    def check_cmos_net(self, ckt_idx, mos_node_idx, db, first, second, third, fourth):
        """
        @brief check the cmos is connected to the correct nets
        """
        ckt = db.subCkt(ckt_idx)

        pin1_idx = ckt.node(mos_node_idx).pinIdx(0)
        net1_idx = ckt.pin(pin1_idx).netIdx
        net1 = ckt.net(net1_idx)
        self.assertEqual(net1.name, first)

        pin2_idx = ckt.node(mos_node_idx).pinIdx(1)
        net2_idx = ckt.pin(pin2_idx).netIdx
        net2 = ckt.net(net2_idx)
        self.assertEqual(net2.name, second)

        pin3_idx = ckt.node(mos_node_idx).pinIdx(2)
        net3_idx = ckt.pin(pin3_idx).netIdx
        net3 = ckt.net(net3_idx)
        self.assertEqual(net3.name, third)

        pin4_idx = ckt.node(mos_node_idx).pinIdx(3)
        net4_idx = ckt.pin(pin4_idx).netIdx
        net4 = ckt.net(net4_idx)
        self.assertEqual(net4.name, fourth)

if __name__ == '__main__':
    unittest.main()