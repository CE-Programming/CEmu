#!/usr/bin/env python
# YAML to (GitHub Actions friendly) JSON
# This reads in a YAML file and returns GitHub Actions friendly JSON to the
# env variable specified. This works by printing this special value:
#   ::set-env name=ENV_VAR_NAME::ENV_VAR_VALUE
# Once you have your JSON, you can call fromJSON(value) to parse it.
# 
# Usage: yml2jsonvar.py VAR_NAME yml_file
# 
# Currently GitHub Actions isn't a fan of multiline strings... see:
#   https://github.community/t/set-output-truncates-multiline-strings/16852
#   https://github.com/actions/toolkit/issues/403
#   https://github.com/actions/toolkit/issues/193
#   https://github.com/actions/toolkit/issues/193

import argparse
import json

from yaml import load, dump
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

def parse_args():
    parser = argparse.ArgParser(description="Parse YAML file into a GitHub Actions env var.")
    parser.add_argument("var_name", help="variable name to store JSON in")
    parser.add_argument("yaml_file", help="YAML file to read from")
    return parser.parse_args()

def yaml_to_json(yaml_file):
    with open(yaml_file, "r") as yaml_fh:
        data = load(yaml_fh, Loader=Loader)
        return json.dumps(data)

if __name__ == "__main__":
    args = parse_args()
    
    json_out = yaml_to_json(args.yaml_file)
    print("::set-env name={var_name}::{json_out}.format(var_name=args.var_name, json_out=json_out)
