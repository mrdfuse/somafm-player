# Lighthugger

<img src="data/io.github.mrdfuse.lighthugger.svg" width="96" align="right" alt="">

A native Qt 6 / Kirigami desktop player for [SomaFM](https://somafm.com/) internet radio.
No browser engine involved.

Named after the lighthuggers from Alastair Reynolds' *Revelation Space*: vast ships
that drift for years just below the speed of light. Made for long stretches of
Drone Zone, Deep Space One and Space Station Soma.

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

    git clone https://github.com/mrdfuse/lighthugger.git
    cd lighthugger/packaging
    makepkg -si

Updating: `git pull`, then run `makepkg -si` again (bump `pkgver` or `pkgrel` in the
PKGBUILD when you change things).

## Build manually

Needs Qt 6 (base, declarative, multimedia), Kirigami and qqc2-desktop-style.

    mkdir build && cd build
    qmake6 ..            # optional: PREFIX=/usr (default is ~/.local)
    make -j$(nproc)
    ./lighthugger        # or: make install

Run `lighthugger [station-id]` to start playing a station right away
(for example `lighthugger dronezone`).

Shortcuts: Ctrl+Space play/stop, Ctrl+←/→ previous/next station, Ctrl+F search.

Favorites and settings live in `~/.config/lighthugger/lighthugger.conf`.

## License

MIT
