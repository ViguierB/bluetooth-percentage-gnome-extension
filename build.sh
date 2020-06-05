if [ "$RELEASE" != 'false' ]; then
  RELEASE='true';
fi

make -C battery_level_engine RELEASE=$RELEASE re

cp battery_level_engine/battery_level_engine bluetooth-percentage@ben.holidev.net/bin/