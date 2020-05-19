#!/bin/bash
time ./producers 4444 300 10.0 1 &
time ./consumers 4444 300 10.0 1
