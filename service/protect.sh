#!/bin/bash
while sleep 1; do
  echo "Fixing perms..."
  find users -type f -exec chown root {} \;
  find users -type d -exec chattr +a {} \;
  echo "Done."
done
