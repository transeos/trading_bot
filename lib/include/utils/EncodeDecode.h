//
// Created by Subhagato Dutta on 9/14/17.
//

#ifndef CRYPTOTRADER_PARAMENCODEDECODE_H
#define CRYPTOTRADER_PARAMENCODEDECODE_H

#include <cryptopp/base64.h>
#include <cryptopp/hex.h>
#include <cryptopp/hmac.h>
#include <cryptopp/osrng.h>

class EncodeDecode {
 public:
  static std::string getHmacSha384(const std::string& key, const std::string& content) {
    using CryptoPP::SecByteBlock;
    using CryptoPP::StringSource;
    using CryptoPP::HexEncoder;
    using CryptoPP::StringSink;
    using CryptoPP::HMAC;
    using CryptoPP::HashFilter;
    using std::transform;
    using std::string;

    string digest;

    SecByteBlock byteKey((const uint8_t*)key.data(), key.size());
    string mac;
    digest.clear();

    HMAC<CryptoPP::SHA384> hmac(byteKey, byteKey.size());
    StringSource ss1(content, true, new HashFilter(hmac, new StringSink(mac)));
    StringSource ss2(mac, true, new HexEncoder(new StringSink(digest)));
    transform(digest.begin(), digest.end(), digest.begin(), ::tolower);

    return digest;
  }

  static std::string getHmacSha256(const std::string& key, const std::string& content, const bool hex_encode = false) {
    using CryptoPP::SecByteBlock;
    using CryptoPP::StringSource;
    using CryptoPP::HexEncoder;
    using CryptoPP::StringSink;
    using CryptoPP::HMAC;
    using CryptoPP::HashFilter;
    using std::transform;
    using std::string;

    string digest;

    SecByteBlock byteKey((const uint8_t*)key.data(), key.size());
    string mac;
    digest.clear();

    HMAC<CryptoPP::SHA256> hmac(byteKey, byteKey.size());
    StringSource ss1(content, true, new HashFilter(hmac, new StringSink(mac)));

    if (hex_encode) {
      StringSource ss2(mac, true, new HexEncoder(new StringSink(digest)));
      transform(digest.begin(), digest.end(), digest.begin(), ::tolower);
      return digest;
    } else
      return mac;
  }

  static std::string getBase64Encoded(const std::string& content) {
    using CryptoPP::StringSource;
    using CryptoPP::Base64Encoder;
    using CryptoPP::StringSink;

    std::string encoded;

    uint8_t buffer[1024] = {};

    for (size_t i = 0; i < content.length(); i++) {
      buffer[i] = content[i];
    };

    StringSource ss(buffer, content.length(), true, new Base64Encoder(new StringSink(encoded), false));

    return encoded;
  }

  static std::string getBase64Decoded(const std::string& content) {
    using CryptoPP::StringSource;
    using CryptoPP::Base64Decoder;
    using CryptoPP::StringSink;

    std::string decoded;

    StringSource ss(content, true,
                    new Base64Decoder(new StringSink(decoded))  // Base64Decoder
                    );                                          // StringSource

    return decoded;
  }
};

#endif  // CRYPTOTRADER_PARAMENCODEDECODE_H
