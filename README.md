# SomaFM Player for KDE

A native Qt 6 / Kirigami desktop player for [SomaFM](https://somafm.com/) internet radio.
No browser engine involved.

> Unofficial app, not affiliated with SomaFM. SomaFM is listener-supported —
> please consider [supporting them](https://somafm.com/support/).

## Features

- All stations with logo, description, genre, listener count and what's on air now
- Favorites, with their own tab
- Station page with DJ, genre, listeners, now playing and recently played tracks
- Search, genre filter, and sorting by name, listeners or genre
- Stream quality: MP3 128k, AAC 128k, AAC+ 64k or 32k. Falls back to mirror servers
  and reconnects when a stream drops
- MPRIS support: Plasma media controls, lock screen and media keys. Next/previous
  cycles through your favorites
- Only one instance runs; starting it again brings the window to the front

## Install (Arch, CachyOS, EndeavourOS, …)

    git clone https://github.com/mrdfuse/somafm-player.git
    cd somafm-player/packaging
    makepkg -si

Updating: `git pull`, then run `makepkg -si` again (bump `pkgver` or `pkgrel` in the
PKGBUILD when you change things).

## Build manually

Needs Qt 6 (base, declarative, multimedia), Kirigami and qqc2-desktop-style.

    mkdir build && cd build
    qmake6 ..            # optional: PREFIX=/usr (default is ~/.local)
    make -j$(nproc)
    ./somafm-player      # or: make install

Run `somafm-player [station-id]` to start playing a station right away
(for example `somafm-player groovesalad`).

Shortcuts: Ctrl+Space play/stop, Ctrl+←/→ previous/next station, Ctrl+F search.

Favorites and settings live in `~/.config/somafm-player/somafm-player.conf`.

## License

MIT
