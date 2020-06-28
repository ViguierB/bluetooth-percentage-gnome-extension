if [ "$RELEASE" != 'false' ]; then
  RELEASE='true';
fi

make -C battery_level_engine RELEASE=$RELEASE re

echo "Copying binary..."
cp battery_level_engine/battery_level_engine bluetooth-percentage@ben.holidev.net/bin/

echo "Copying icons..."
( cd logo_bluetooth_battery_inkscape; ./copy.sh )