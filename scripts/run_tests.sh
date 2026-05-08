#!/bin/bash
  

if [ ! -f "platformio_test.ini" ]; then
printf "[env:latest_stable]\n" > platformio_test.ini
printf "platform = native\n" >> platformio_test.ini
printf "lib_deps = Unity, ./\n" >> platformio_test.ini
printf "build_flags = \n\t-Itest/include" >> platformio_test.ini
printf "\t-DEAGLETRT_STATIC=\n" >> platformio_test.ini
printf "\t-DEAGLETRT_STATIC_INLINE=\n" >> platformio_test.ini
echo "Created platformio_test.ini"
fi

pio test -e latest_stable --project-conf platformio_test.ini

rm platformio_test.ini
echo "Removed platformio_test.ini"