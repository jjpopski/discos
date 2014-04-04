#!/usr/bin/env python
# Author: Marco Buttu <m.buttu@oa-cagliari.inaf.it>
# Copyright: This module has been placed in the public domain.
"""Save the data in order to plot the real elongations during the scan"""

import unittest
import threading
import os
import time
from random import randrange as rr
from socket import *       
from Acspy.Clients.SimpleClient import PySimpleClient
from Acspy.Common import TimeHelper
from maciErrTypeImpl import CannotGetComponentExImpl
import datetime

class NackEx(Exception):
    pass

class TestFocusScan(unittest.TestCase):
    """Test the minor servo startFocusScan() method."""

    def setUp(self):
        self.pyclient = PySimpleClient()
        self.cmd_num = 0
        self.srp = self.pyclient.getComponent("MINORSERVO/SRP")
        self.boss = self.pyclient.getComponent("MINORSERVO/Boss")

    def tearDownClass():
        print "in tearDown"
        self.sockobj.close()
        self.pyclient.releaseComponent(self.srp)
        self.pyclient.releaseComponent(self.boss)

    def test_startScan(self):
        """Test the startFocusScan() method (SRP active)."""
        setup_code = 'CCB'
        self.boss.setup(setup_code)
        print "\nExecuting the %s setup. Wait a bit ..." %setup_code
        time.sleep(1) # Wait a bit, until the boss begins the configuration process

        counter = 0
        delay_ready = 2
        while not self.boss.isReady() or not self.srp.isReady(): # Wait until the minor servo boss is ready
            time.sleep(delay_ready) # Wait a bit, until the boss is active
            counter += delay_ready
            if counter > 240:
                self.assertFalse(counter > 240)
                return

        print "\nThe MinorServoBoss is ready."

        time.sleep(2)
        delay = 10 * 10 ** 7 # 10 seconds
        starting_time = TimeHelper.getTimeStamp().value + delay # Start the scan in `delay` seconds from now
        range_ = 15.0 # mm 
        total_time = 5 * 10 ** 7 # 5 seconds
        mm_per_sec = range_ / total_time
        actPos_obj = self.srp._get_actPos()
        actPos, cmp = actPos_obj.get_sync()
        print "\nCalling the startScan() method, starting in %d seconds from now." %(delay/10**7)
        self.boss.startFocusScan(starting_time, range_, total_time)
        expected_positions = {}
        sampling_time = 2000000 # 200ms
        exe_times = range(starting_time, starting_time + total_time + sampling_time, sampling_time)
        expected_position = actPos[:]
        for i, t in enumerate(exe_times):
            temp_pos = actPos[2] -range_/2 + (t - starting_time) * mm_per_sec
            if temp_pos > actPos[2] + range_/2:
                expected_position[2] = actPos[2] + range_/2
                print 'expected[2]: ', expected_position[2]
            else:
                expected_position[2] = actPos[2] -range_/2 + (t - starting_time) * mm_per_sec
            expected_positions[t] = expected_position[:]
        for i in range(1, 5):
            expected_positions[t + sampling_time * i] = expected_position[:]

        actual_time = TimeHelper.getTimeStamp().value 
        print "\nWaiting for the scan..."
        while actual_time < starting_time:
            actual_time = TimeHelper.getTimeStamp().value 
            time.sleep(0.2)

        print "\nScanning..."
        while actual_time < starting_time + total_time:
            actual_time = TimeHelper.getTimeStamp().value 
            time.sleep(0.2)
        print "\nDone!"

        print "Writing the file..."
        out_file = open('../data/scan.data', 'w')
        out_file.write('# Running time: %s\n\n' %str(datetime.datetime.now()))
        out_file.write('# Starting time: %d\n' %starting_time)
        out_file.write('# Total time: %d\n\n s' %(total_time/10**7))
        out_file.write('# Range: %d mm\n\n' %range_)
        out_file.write('# History sampling time: %d\n' %sampling_time)

        hpositions = {}
        for t in sorted(expected_positions.keys()):
            ep = ' '.join(['%.4f' %item for item in expected_positions[t]])
            hp = self.srp.getPositionFromHistory(t)
            out_file.write("\n#" + "-"*68)
            out_file.write("\nTime: starting time + %0*d ms" %(5, (t - starting_time) / (10**4)))
            out_file.write("\nExpected: %s" %ep)
            out_file.write("\nHistory:  %s" %' '.join(['%.4f' %item for item in hp]))
            hpositions[t] = hp

        self.boss.stopScan()

        pos = [expected_positions[t] for t in sorted(expected_positions.keys())]
        hpos = [hpositions[t] for t in sorted(hpositions.keys())]
        max_diff = max([abs(p[2] - h[2]) for p, h in zip(pos, hpos)])
        out_file.write("\n\n# Max diff: %.4f" %max_diff)
        print "Max diff: ", max_diff
        print "Done!"

if __name__ == "__main__":
    suite = unittest.TestLoader().loadTestsFromTestCase(TestFocusScan)
    unittest.TextTestRunner(verbosity=2).run(suite)
    print "\n" + "="*70

