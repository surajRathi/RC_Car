#! /usr/bin/bash

# TODO: This should be a Makefile
find . -type f -name "*.html" -exec sh -c 'xxd -i $(echo {} | sed "s|^\./||") | tee $(echo "{}" | sed "s:\(.*\)\.:\1_:" ).h > /dev/null' \;