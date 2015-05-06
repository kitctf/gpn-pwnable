#! /usr/bin/python

# ################################################################
#
# Gulasch hut Gameserver Script
#
# author: mp@floatec.de
#
#################################################################


import os
import random
import re
import subprocess
import sys
import requests
import string
import pickle


HTTPORT = ":80"
BASEDIR = "/"
TIMEOUT = 5  # for each connection! Keep in mind that the gameserver script
# connects up to four times each round.

# return values
SUCCESS = 0
GENERIC = 1
FLAGERR = 5
GARBLED = 9
NETWORK = 13
FOULERR = 21





##
# Print a message on stderr
##
def error(msg):
    sys.stderr.write(str(msg) + "\n")


##
# Get random string of n characters 
##
def randdomString(n):
    return ''.join(random.SystemRandom().choice(string.ascii_uppercase + string.digits) for _ in range(n))


##
# Store a flag
##
def store(ip, id, flag):
    print("SUCCESS")
    return SUCCESS;


##
# Retrieve a flag
##
def retrieve(ip, id, flag):
    print("SUCCESS")
    return SUCCESS;


##
# Main
##
def main():
    if len(sys.argv) < 5:
        error("Usage: " + sys.argv[0] + " $action $ip $id $flag")
        print("Internal gameserver script error")
        return GENERIC

    action = sys.argv[1]
    ip = sys.argv[2]
    id = sys.argv[3]
    flag = sys.argv[4]

    if (action == "store"):
        return store(ip, id, flag)
    elif (action == "retrieve"):
        return retrieve(ip, id, flag)
    else:
        error("$action has to be 'store' or 'retrieve'")
        print("Internal gameserver script error")
        return GENERIC


sys.exit(main())