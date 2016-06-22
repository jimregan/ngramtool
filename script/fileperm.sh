#!/bin/sh
# fix file permissions
find . -type f -exec chmod 0644 {} \;
chmod 0755 configure
chmod 0755 script/*
