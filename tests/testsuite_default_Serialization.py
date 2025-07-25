# -*- coding: utf-8 -*-
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import os
import filecmp

from sst_unittest import *
from sst_unittest_support import *


class testcase_Serialization(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_Serialization_pod(self):
        self.serialization_test_template("pod")

    def test_Serialization_pod_ptr(self):
        self.serialization_test_template("pod_ptr")

    def test_Serialization_array(self):
        self.serialization_test_template("array")

    def test_Serialization_ordered_containers(self):
        self.serialization_test_template("ordered_containers")

    def test_Serialization_unordered_containers(self):
        self.serialization_test_template("unordered_containers")

    def test_Serialization_map_to_vector(self):
        self.serialization_test_template("map_to_vector")

    def test_Serialization_optional(self):
        self.serialization_test_template("optional")

    def test_Serialization_pointer_tracking(self):
        self.serialization_test_template("pointer_tracking")

    def test_Serialization_handler(self):
        self.serialization_test_template("handler", False)

    def test_Serialization_component_info(self):
        self.serialization_test_template("componentinfo", False)

    def test_Serialization_atomic(self):
        self.serialization_test_template("atomic")

    def test_Serialization_complexcontainer(self):
        self.serialization_test_template("complexcontainer")

    def test_Serialization_variant(self):
        self.serialization_test_template("variant")

#####
    def serialization_test_template(self, testtype, default_reffile = True):

        testsuitedir = self.get_testsuite_dir()
        outdir = test_output_get_run_dir()

        sdlfile = "{0}/test_Serialization.py".format(testsuitedir)
        if default_reffile:
            reffile = "{0}/refFiles/test_Serialization_default.out".format(testsuitedir)
        else:
            reffile = "{0}/refFiles/test_Serialization_{1}.out".format(testsuitedir,testtype)

        outfile = "{0}/test_Serialization_{1}.out".format(outdir,testtype)

        options = "--run-mode=init --model-options=\"{0}\"".format(testtype)
        # Force serial run since the serialization is all done in-situ
        self.run_sst(sdlfile, outfile, num_ranks=1, num_threads=1, other_args=options)

        # Perform the test
        filter1 = StartsWithFilter("WARNING: No components are")
        cmp_result = testing_compare_filtered_diff("serialization", outfile, reffile, True, [filter1])
        if not cmp_result:
            diffdata = testing_get_diff_data(testtype)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
