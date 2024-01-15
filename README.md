# Map Quiz

A simple quiz to learn states.

## Quick Start

```console
$ ./build.sh && ./quiz
```

The application needs `resources` to be present in the folder. 

## Controls

- `Mouse left button` -- select
- `Mouse right button` -- drag
- `Mouse wheel` -- zoom 
- `r` -- restart 
- `l` -- learn
- `q` -- quit 

## Dependencies

- [Raylib 5.0](https://github.com/raysan5/raylib)

## Data Source

- [Brazil map](https://vemaps.com/brazil/br-07#google_vignette)
- [Mexico map](https://www.geoguessr.com/pdf/4078)
- [Japan map](https://vemaps.com/japan/jp-02)
- [Phillipines islands map](https://vemaps.com/phillipines/ph-01)
- [Malaysia map](https://vemaps.com/malaysia/my-02)

# Developer notes

The project can be compiled to WebAssembly to run on Web using `emscripten SDK`.
The proper environment needed to find the `emcc` compiler provided by `emscripten`. Then WebAssembly can be used as a target in the `build` script. 

```console 
source ./emsdk/emsdk_env.sh
```

See the further [explanation](https://github.com/raysan5/raylib/wiki/Working-for-Web-(HTML5)#3-build-examples-for-the-web) provided by raysan.  
