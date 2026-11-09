## Overview

This directory contains various CPU tests that have been created with the cputester tool written by Toni Wilen. cputester is part of UAE. The tests are located in two different sub-directories:

## ADF

This directory contains tests in ADF format. All disks have been set up to auto-boot and test a single instruction each. Storing tests on ADFs is not ideal, because it is sometimes difficult to make a test fit on a single disk. However, the tests have been generated at a time where vAmiga had no hard drive capabilities, so I had no choice. 

## HDF

The tests in this directory have been created with a newer version of cputester. At that time, vAmiga had already gotten hard-drive support, so I was able to put all tests into a single HDF. Using cputester in combination with HDFs vastly simplifies the test procedure. 
