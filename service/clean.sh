#!/bin/bash
find users -type d -exec chattr -a {} \;
chattr -a users
rm -r users
