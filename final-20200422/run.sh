#!/bin/bash
time ./producers 4443 400 10.0 25 &
time ./consumers 4443 400 10.0 25
