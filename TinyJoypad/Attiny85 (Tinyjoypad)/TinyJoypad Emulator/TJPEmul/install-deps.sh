#!/usr/bin/env bash
set -euo pipefail

echo "=== Installation des dépendances TinyJoypad ==="

if [ ! -f /etc/os-release ]; then
    echo "Impossible de détecter la distribution Linux."
    exit 1
fi

. /etc/os-release
DISTRO="${ID:-}"

install_debian() {
    sudo apt update
    sudo apt install -y \
        build-essential git pkg-config cmake \
        libsdl2-dev libelf-dev libglib2.0-dev

    # Le paquet de développement n'est pas disponible dans toutes les versions.
    if apt-cache show libsimavr-dev >/dev/null 2>&1; then
        sudo apt install -y libsimavr-dev
    fi
}

install_fedora() {
    sudo dnf install -y \
        gcc gcc-c++ make git pkgconf cmake \
        SDL2-devel elfutils-libelf-devel glib2-devel

    if sudo dnf info simavr-devel >/dev/null 2>&1; then
        sudo dnf install -y simavr-devel
    elif sudo dnf info simavr >/dev/null 2>&1; then
        sudo dnf install -y simavr
    fi
}

install_arch() {
    sudo pacman -Syu --noconfirm \
        base-devel git pkgconf cmake \
        sdl2 libelf glib2

    if pacman -Si simavr >/dev/null 2>&1; then
        sudo pacman -S --needed --noconfirm simavr
    fi
}

case "$DISTRO" in
    ubuntu|debian|linuxmint|pop|elementary)
        install_debian
        ;;
    fedora|rhel|centos|rocky|almalinux)
        install_fedora
        ;;
    arch|manjaro|endeavouros)
        install_arch
        ;;
    *)
        echo "Distribution non prise en charge automatiquement : $DISTRO"
        echo "Installez manuellement gcc, make, git, SDL2, libelf et simavr."
        exit 1
        ;;
esac

find_simavr_header() {
    find /usr/include /usr/local/include \
        -name sim_avr.h -print -quit 2>/dev/null
}

# Si le paquet n'a pas fourni les en-têtes, installer simavr depuis les sources.
if [ -z "$(find_simavr_header)" ]; then
    echo "sim_avr.h absent : installation de simavr depuis les sources."

    tmpdir="$(mktemp -d)"
    trap 'rm -rf "$tmpdir"' EXIT

    git clone --depth 1 https://github.com/buserror/simavr.git "$tmpdir/simavr"
    make -C "$tmpdir/simavr" -j"$(nproc)"
    sudo make -C "$tmpdir/simavr" install
    sudo ldconfig
fi

header="$(find_simavr_header)"
if [ -z "$header" ]; then
    echo "Échec : sim_avr.h reste introuvable."
    exit 1
fi

echo "En-tête simavr trouvé : $header"
echo "Installation terminée. Lancez : make"
