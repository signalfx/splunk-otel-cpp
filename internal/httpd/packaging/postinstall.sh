#!/bin/sh

set -e

a2enmod opentelemetry
apachectl restart
