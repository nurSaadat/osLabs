#!/bin/bash
time ./producers 4444 150 10.0 75 &
time ./consumers 4444 150 10.0 75 &
