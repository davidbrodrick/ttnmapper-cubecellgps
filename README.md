# ttnmapper-cubecellgps

This is an Arduino sketch for the [https://github.com/HelTecAutomation/ASR650x-Arduino](Heltec CubeCell GPS) to upload to [https://ttnmapper.org/](TTN Mapper).

This implementation uses the [LoRa-Serialization](https://github.com/thesolarnomad/lora-serialization) library, but it would be trivial enough to encode the lat,long,hdop,altitude yourself. With this library, the decoder on TTN looks like this:

```
function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  return decode(bytes, [uint8, latLng, uint16], ['hdop', 'coords', 'altitude']);
}
```
