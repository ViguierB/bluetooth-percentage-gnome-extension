if [ -e "../bluetooth-percentage@ben.holidev.net/icons" ]; then
  rm -r ../bluetooth-percentage@ben.holidev.net/icons
fi


(
  mkdir ../bluetooth-percentage@ben.holidev.net/icons;
  cd ./out;
  for old in *; do
    echo "converting $old";
    new="../../bluetooth-percentage@ben.holidev.net/icons/$old"
    rsvg-convert "$old" -w 16 -h 16 -f svg -o "$new"
  done
)