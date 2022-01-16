#!/bin/sh

echo -n "TOKEN: "
read token

echo -n "SECRET: "
read secret

touch line.env
echo "TOKEN=$token" >> line.env
echo "SECRET=$secret" >> line.env