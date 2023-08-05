# Game Carrier Examples

This repo containg examples of using The Garrier Framework

## Obtain example sources

To obtain source just execute to following commnds in a terminal:
```bash
    git clone git@github.com:GameCarrier/core-examples.git
    cd examples
 ```

Game Carrier platform developer license file `authentification.dat` should be stored in working directory.
By default the Game Carrier working directory is located in `$HOME/.gc`.

## Build examples

Pre-requisites: `cmake`.
On Windows Visual Studio 2017 or more recent is needed.

To build C & C++ minimal app example with client use the following commands:

```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release -j1
```

To build .NET Core and .NET Framework examples use Visual Studio.
Just open `net-core/net-core.sln` or `net-framework/net-framework.sln` and select Build All from a menu.

During build the configuration filed needed for launching an application is placed to `build/configs` directory.
For C/C++ the `cmake` is used for this.
For .NET the project it is performed by `ConfigPatcher` project.

## Game Carrier `minimal_app` Example

`minimal_app` example provides basic example of minimal application for Game Carrier platform.
Let's assume that the Game Carrier installed to the default location (`/usr/local/bin` on Unix or `C:\Program Files (x86)\Game Carrier\Bin` on Windows).
To run `minimal_app` example use one fro the following commands:

```bash
    # C example:
    /usr/local/bin/gcs -c1 -C build/configs/c-minimal-app.json

    # .NET Core example:
    /usr/local/bin/gcs -c1 -C build/configs/net-core-minimal-app.json

    # .NET Framework example:
    /usr/local/bin/gcs -c1 -C build/configs/net-framework-minimal-app.json
```

To run a client use one of the following commands:

```bash
    # C example:
    build/bin/clients/Release/spammer -h wss://localhost:7681/minimal_app 3

    # .NET Core example:
    build/bin/clients/Release/net-core/Spammer -h wss://localhost:7681/MinimalApp 3

    # .NET Framework example:
    build/bin/clients/Release/net-framework/Spammer -h wss://localhost:7681/MinimalApp 3
```

In all cases successfuly run spammer application will produce console run log output with similar lines:

```text
    ......
    [2022/12/15 17:34:11:3205] U: #0 R: app-0 received 131843 bytes.
    [2022/12/15 17:34:11:3340] U: #0 R: app-0 received 131789 bytes.
    [2022/12/15 17:34:11:3470] U: #0 R: app-0 received 131964 bytes.
    ......
```
