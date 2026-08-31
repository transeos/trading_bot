# README #

## Building

- **Linux:** see [SETUP.md](SETUP.md).
- **Windows (MSVC / Visual Studio + vcpkg):** see the
  [Windows Setup](SETUP.md#windows-setup-msvc--visual-studio--vcpkg) section of SETUP.md.

## Configuration

# populate configs/static_config.json
"exchanges": {
  "coinbase": {
    "api-credential": {
      "key": "",
      "secret": "",
      "passphrase": ""
    },

"coinapi": {
  "api-credential": [
    {
      "key": ""
    },
  ]
}

# populate "data/gemini_data" if needed

# example trading algo
  lib/src/tradeAlgos

# add new trading algo entry in
  lib/src/IncludeAlgo.cpp
