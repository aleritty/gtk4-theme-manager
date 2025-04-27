# GTK4 Theme Manager

A modern, user-friendly application for managing GTK4 themes on Linux desktop environments.

![GTK4 Theme Manager](favicon.svg)

## Overview

GTK4 Theme Manager is a lightweight application that allows you to:

- Browse and preview installed GTK4 themes
- Apply themes to your desktop environment
- View detailed theme metadata and suggested configurations
- Install new themes by drag-and-drop
- Delete unwanted themes

Built with GTK4, this application provides a clean, modern interface for managing your desktop appearance.

## Features

- **Theme Browsing**: View all installed GTK4 themes in an organized sidebar
- **Theme Preview**: See a visual preview of each theme before applying
- **Metadata Display**: View detailed information about each theme, including suggested configurations
- **Drag-and-Drop Installation**: Install new themes by simply dragging theme archives onto the application
- **Theme Management**: Apply or delete themes with a single click
- **Real-time Updates**: Automatically detects when themes are added or removed

## Screenshots

*[Screenshots will be added here]*

## Installation

### Dependencies

- GTK4 (>= 4.0)
- GLib/GIO (>= 2.0)

### From Source

Clone the repository and build from source:

```bash
# Clone the repository
git clone https://github.com/yourusername/gtk4-theme-manager.git
cd gtk4-theme-manager

# Build the application
./script.sh build

# Run the application
./script.sh run

# Or instal it
./script.sh install
```

### Arch Linux (and derivatives)

```bash
# Manually
makepkg -si
```

### Debian/Ubuntu

Download the latest `.deb` package from the [Releases](https://github.com/yourusername/gtk4-theme-manager/releases) page and install:

```bash
sudo dpkg -i theme-manager.deb
```

## Usage

### Starting the Application

Launch the application from your application menu or run:

```bash
theme-manager
```

### Browsing Themes

1. The left sidebar displays all installed GTK4 themes
2. Click on a theme to view its details and preview

### Applying a Theme

1. Select a theme from the sidebar
2. Click the "Apply Theme" button in the theme details view

### Installing New Themes

1. Download a GTK theme archive (`.tar.gz`, `.tar.xz`, etc.)
2. Drag and drop the archive onto the application window
3. The theme will be automatically extracted to `~/.themes`

### Removing Themes

1. Select a theme from the sidebar
2. Click the "Delete Theme" button in the theme details view
3. Confirm the deletion in the dialog

## Building from Source

### Requirements

- GCC or compatible C compiler
- GTK4 development libraries
- pkg-config

### Build Instructions

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install gcc libgtk-4-dev pkg-config

# Install dependencies (Arch Linux)
sudo pacman -S gcc gtk4 pkgconf

# Build
gcc $(pkg-config --cflags gtk4) -o theme-manager theme-manager.c $(pkg-config --libs gtk4)
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the GPL License - see the LICENSE file for details.

## Acknowledgments

- The GTK team for the excellent GTK4 toolkit
- All the theme creators who make our desktops beautiful
