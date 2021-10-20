#!/usr/bin/env python3
import json

with open('creport10.json') as fd:
    jsdata = json.load(fd)
    for k, d in jsdata.items():
        if isinstance(d, int):
            print("{:16} {}".format(k, d))
        elif isinstance(d, list):
            ct = 0
            print(k)
            for i in d:
                print("   {} '{}'".format(ct, i))
                ct = ct + 1
        else:
            print("{:16} '{}'".format(k, d))

