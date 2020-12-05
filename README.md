# ttnmapper-cubecellgps

This is an Arduino sketch for the [Heltec CubeCell GPS](https://github.com/HelTecAutomation/ASR650x-Arduino) to upload to [TTN Mapper](https://ttnmapper.org/).

This implementation uses the [LoRa-Serialization](https://github.com/thesolarnomad/lora-serialization) library, but it would be trivial enough to encode/decode the lat,long,hdop,altitude yourself if you wanted to get rid of the dependency. With this library, the decoder on TTN looks like this:

```
function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  return decode(bytes, [uint8, latLng, uint16], ['hdop', 'coords', 'altitude']);
}

//Plus the standard LoRa-Serialization functions which go below here
```
