#!/bin/bash
#
# Run this script regularly to protect malicious users from tampering with
# other user data
find users -type f -exec chown root {} \;
find users -type d -exec chattr +a {} \;
