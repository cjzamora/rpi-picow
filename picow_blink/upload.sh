# if build directory does not exists, create it
if [ ! -d "build" ]; then
  mkdir build
  cd build
  cmake ..
  make

# else build and upload
else
  cd build
  make

  # find the .uf2 file
  UF2=$(find . -name "*.uf2")
  VOL=/Volumes/RPI-RP2

  echo " "
  echo "Uploading $UF2 to $VOL..."
  rsync $UF2 $VOL
  echo "Done!"
fi