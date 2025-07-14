#!/bin/bash

# Exit on error
set -e

# Paths
BUILD_DIR="/home/lasitha/JLab-analyzers/moller-counting/build"
INSTALL_DIR="/home/lasitha/JLab-analyzers/moller-counting-install"
SRC_DIR="/home/lasitha/JLab-analyzers/moller-counting"

echo "🔄 Cleaning build directory..."
cd "$BUILD_DIR"
rm -rf *

echo "🧹 Cleaning install directory (requires sudo)..."
sudo rm -rf "${INSTALL_DIR:?}"/*

echo "⚙️ Running cmake..."
cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" ..

echo "🔨 Building with make..."
make -j8

echo "📦 Installing to $INSTALL_DIR (requires sudo)..."
sudo make install

echo "📗 Sourcing environment setup..."
source "$INSTALL_DIR/bin/mollerenv.sh"
source "$SRC_DIR/MOLLER-replay/replay/setup_ana_env.sh"

echo "📚 Copying .rootrc from install to replay directory..."
cp "${INSTALL_DIR}/run_replay_here/.rootrc" "${SRC_DIR}/MOLLER-replay/replay"

echo "✅ Done! MOLLER analyzer rebuilt and environment sourced."
