#!/bin/sh
#
# ROOT=/nas/data/Development/Raspberry/gpiocrtl/test-install
#

if [ `id -u` != 0 ]; then
 echo "Must be run as root"
 exit 2
fi

ROOT=/

#install
#
#copy init-script
#
#copy cfg
#/etc/gpioctrl/gpioctrl.cfg
#
#copy binary
#/usr/local/bin
#
#copy webdefault
#/var/www-gpioctrl
#
#update-rc.d gpioctrld defaults
#

BUILD=.

CFG=$ROOT/etc/gpioctrl/
WEB=$ROOT/var/www-gpioctrld/
BIN=$ROOT/usr/local/bin

mkdir -p $CFG
mkdir -p $WEB
mkdir -p $BIN

if [ -f $ROOT/etc/init.d/gpioctrld ]; then
  $ROOT/etc/init.d/gpioctrld stop
  SERVICE=1
fi

cp $BUILD/gpioctrl $BIN
cp $BUILD/gpioctrld $BIN
cp -r $BUILD/htdocs/* $WEB
cp $BUILD/gpioctrld.init-d $ROOT/etc/init.d/gpioctrld

if [ -f $CFG/gpioctrl.conf ]; then
  echo "Old config exists, did not copy new-blank config. To use new config copy $BUILD/config.cfg to $CFG/gpioctrl.conf and modify as needed."
else
  cp $BUILD/config.cfg $CFG/gpioctrl.conf
fi

update-rc.d gpioctrld defaults

if [ $SERVICE -eq 1 ]; then
  $ROOT/etc/init.d/gpioctrld start
fi

exit

DEPLOYEDFILES='racingIndexer.jar racingIndexer.sh racingIndexer.prop.example'

for file in $DEPLOYEDFILES; do
  echo Getting $file
  curl -o "$LIB/$file" "https://raw.githubusercontent.com/sfeakes/RacingIndexer/master/deploy/$file"
done

echo 

# link bin to script
if [ -e $BIN/$NAME ]; then
  if [ ! -L $BIN/$NAME ]; then
    echo "Error $BIN/$NAME exists but is not a link to $LIB/$SHNAME"
  fi
else
  ln -s $LIB/$SHNAME $BIN/$NAME
fi

if [ ! -f $LIB/$PROPNAME ]; then
  echo "Please copy $LIB/$PROPNAME.example to $LIB/$PROPNAME and modify as needed."
fi
