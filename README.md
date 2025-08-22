# Shop scheduling solver

A solver for flow shop, job shop and open shop scheduling problems.

## Compilation

```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel && cmake --install build --config Release --prefix install
```

## Implemented algorithms

$F_m \mid \mathrm{prmu} \mid C_{\max}$
* Tree search

$F_m \mid \mathrm{prmu} \mid \sum C_j$
* Tree search
