#!/usr/bin/env bash

cd analysis
./build.sh
cd ..
python live_var_app.py