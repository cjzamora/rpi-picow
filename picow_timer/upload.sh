# success flag
SUCCESS=0
# if build directory does not exists, create it
if [ ! -d "build" ]; then
  mkdir build && cd build && cmake .. && make && SUCCESS=1
# else build and upload
else
  cd build && make && SUCCESS=1
fi

# find the .uf2 file
UF2=$(find . -name "*.uf2")
VOL=/Volumes/RPI-RP2
PORT=/dev/cu.usbmodem141101

echo " "

# if not successful, exit
if [ $SUCCESS -eq 0 ]; then
  echo "Build failed!"
  exit 1
fi

# reboot if serial port exists
if [ -e $PORT ]; then
  echo "Rebooting RP2040..."
  echo "bsel" > $PORT && sleep 5
fi

UPLOADED=0
echo "Uploading $UF2 to $VOL..."
rsync $UF2 $VOL && UPLOADED=1

# if not uploaded, exit
if [ $UPLOADED -eq 0 ]; then
  echo " "
  echo "Upload failed!"
  exit 1
fi

echo "Upload success!"