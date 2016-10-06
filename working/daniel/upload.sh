#!/bin/sh

RCOM_DIR=/home/danielf/src/RCOM2/

cd $RCOM_DIR
echo $PWD
tar cf rcom.tar RCOM
mv rcom.tar /var/www/danielf/pub/tmp
echo finished
