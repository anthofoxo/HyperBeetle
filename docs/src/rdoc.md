# RenderDoc
We have built in support for renderdoc. Meaning if the application is luanched via renderdoc, we can interact with renderdoc. Additionally if the application is launched outside of renderdoc. The engine will attempt to automcatically attach it.

## Renderdoc search other
This applies when the application was not ran via renderdoc. Windows currently supports this better.

### Windows
* `renderdoc.dll`
* `%ProgramFiles%/RenderDoc/renderdoc.dll`

### Unix
* `librenderdoc.so`

## Ui changes
The application disables the renderdoc gui. This ui gets quite intrusive when using the builtin tooling. Renderdoc has a debug ui which may be accessed in the engine.

## Frame capture
`F12` and `PRTSC` are commonly used to take screenshots, thus these defaults of RenderDoc are changed. Use `F2` to take a frame capture.

## Opening captures
The debug ui will contains a list of captures. If you're running windows. You can simply click an item in this list to open renderdoc.