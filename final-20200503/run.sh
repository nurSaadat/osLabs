#!/bin/bash
time ./producers 4444 600 12.0 0 &
time ./consumers 4444 600 12.0 0
