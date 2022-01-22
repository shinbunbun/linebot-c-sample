#!/bin/sh

echo -n "TOKEN: "
read token

echo -n "SECRET: "
read secret

echo "TOKEN=$token" >> line.env
echo "SECRET=$secret" >> line.env