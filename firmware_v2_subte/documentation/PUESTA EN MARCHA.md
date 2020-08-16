# Puesta en marcha:

Para grabar el firmware de la EDUCIAA:
Bajar el firmware “firmware_v2_TP2.tar.gz” y descomprimirlo. Luego:

* cd firmware_v2
* make clean
* make
* make download

**NOTA**: En el caso de que aparezca el error “command not found” se deberá agregar a la variable de
entorno PATH, la ruta al compilador arm: export PATH=$PATH:/home/usuario/ruta_compiler.


**En el firmware_v3, los paths a los binarios que no encuentra están dentro del launcher de software que utilizamos.**

Ejemplo:

*Antes de ejecutar la secuencia anterior deberian ejecutar:*

* export PATH=$PATH:/home/luchao90/CIAA/CIAA_Software_1.1-linux-x64/tools/gcc-arm-embedded/bin
* export PATH=$PATH:/home/luchao90/CIAA/CIAA_Software_1.1-linux-x64/tools/openocd/bin


