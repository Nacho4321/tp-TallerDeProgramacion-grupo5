#!/bin/bash
set -e

AUTO_ASCII=$(cat << 'EOF'
                                 _.-=" _         _
                         _.-="   _-          | ||"""""""---._______     __..
             ___.=== """-.______-,,,,,,,,,,,,`-''----" """"       """"  __'
      __.--""     __        ,'                   o \           __        [__|
 __-""=======.--""  ""--.=================================.--""  ""--.=======:
]       [w] : /        \ : |========================|    : /        \ :  [w] :
V___________:|          |: |========================|    :|          |:   _-"
 V__________: \        / :_|=======================/_____: \        / :__-"
 -----------'  "-____-"  `-------------------------------'  "-____-"
      NEED FOR SPEED INSTALLER
EOF
)

echo "$AUTO_ASCII"

INSTALL_DIR_ASSETS="/var/need_for_speed"
INSTALL_DIR_CONFIG="/etc/need_for_speed"

if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
else
    SUDO="sudo"
fi

export DEBIAN_FRONTEND=noninteractive

install_game() {

    echo "[1/6] Actualizando paquetes..."
    $SUDO apt-get update -y

    echo "[2/6] Instalando dependencias..."
    $SUDO apt-get install -y \
        build-essential cmake git \
        qt6-base-dev qt6-base-dev-tools \
        libopus-dev libopusfile-dev libbox2d-dev\
        libxmp-dev \
        libfluidsynth-dev fluidsynth \
        libwavpack1 libwavpack-dev \
        libfreetype-dev \
        nlohmann-json3-dev \
    || true

    echo "[3/6] Compilando proyecto..."

    # Asegurar que estamos en la carpeta raíz del repo
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    cd "$SCRIPT_DIR"
    
    $SUDO rm -f CMakeCache.txt
    $SUDO rm -rf CMakeFiles
    $SUDO rm -f cmake_install.cmake
    $SUDO rm -rf _deps
    $SUDO rm -f ../CMakeCache.txt
    $SUDO rm -rf ../CMakeFiles/
    $SUDO rm -rf build
    mkdir build
    cd build

    cmake "$SCRIPT_DIR" \
        -DTALLER_CLIENT=ON \
        -DTALLER_CLIENT_UI=ON \
        -DTALLER_SERVER=ON \
        -DTALLER_EDITOR=ON \
        -DTALLER_TESTS=OFF

    make -j"$(nproc)"

    echo "[4/6] Instalando binarios en /usr/bin..."
    $SUDO install -m 755 taller_client /usr/bin/taller_client 2>/dev/null || true
    $SUDO install -m 755 taller_client_ui /usr/bin/taller_client_ui 2>/dev/null || true
    $SUDO install -m 755 taller_server /usr/bin/taller_server 2>/dev/null || true
    $SUDO install -m 755 taller_editor /usr/bin/taller_editor 2>/dev/null || true

    echo "[5/6] Copiando assets a $INSTALL_DIR_ASSETS..."
    $SUDO mkdir -p "$INSTALL_DIR_ASSETS"
    $SUDO cp -r ../data/* "$INSTALL_DIR_ASSETS/"

    echo "[6/6] Copiando config a $INSTALL_DIR_CONFIG..."
    $SUDO mkdir -p "$INSTALL_DIR_CONFIG"
    $SUDO cp -r ../config/* "$INSTALL_DIR_CONFIG/"

    echo ""
    echo "======================================="
    echo " Instalación completa."
    echo " Binarios: /usr/bin/"
    echo " Assets:   $INSTALL_DIR_ASSETS/"
    echo " Config:   $INSTALL_DIR_CONFIG/"
    echo "======================================="
}

uninstall_game() {
    echo "[1/3] Eliminando binarios..."
    $SUDO rm -f /usr/bin/taller_client
    $SUDO rm -f /usr/bin/taller_client_ui
    $SUDO rm -f /usr/bin/taller_server
    $SUDO rm -f /usr/bin/taller_editor

    echo "[2/3] Eliminando assets..."
    $SUDO rm -rf "$INSTALL_DIR_ASSETS"

    echo "[3/3] Eliminando configuración..."
    $SUDO rm -rf "$INSTALL_DIR_CONFIG"

    echo ""
    echo "======================================="
    echo " Desinstalación completada."
    echo "======================================="
}

echo "Seleccione una opción:"
echo "  1) Instalar"
echo "  2) Desinstalar"
echo "  3) Salir"
read -p "> " OPCION

case "$OPCION" in
    1) install_game ;;
    2) uninstall_game ;;
    3) echo "Saliendo..." ; exit 0 ;;
    *) echo "Opción inválida." ; exit 1 ;;
esac
