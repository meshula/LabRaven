
### Building

In this directory, run zig build

labraventest will appear in zig-out/bin so

```
cp zig-out/bin/labraventest /path/to/build/raven.app/Contents/MacOS/plugin/labraventest.so
```

Build labraven, which will codesign raven and the plugin.

### Running

Run labraven, and click the Activities menu. This will cause all the plugins including this one to load, and appear in the Plugins menu. It'll turn red if the load failed for some reason, otherwise, it will appear in the UI panel, announce it's name and filename, and have a checkbox indicating its load success.

### CBB

The plugin is copied to the plugins folder automatically.

