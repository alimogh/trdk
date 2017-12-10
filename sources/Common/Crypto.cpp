/*******************************************************************************
 *   Created: 2017/10/29 18:36:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Crypto.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;

////////////////////////////////////////////////////////////////////////////////

std::string Base64::Encode(const unsigned char *source,
                           size_t sourceLen,
                           bool multiline) {
  const auto &free = [](BIO *bio) {
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);
  };
  const std::unique_ptr<BIO, decltype(free)> bio(
      BIO_push(BIO_new(BIO_f_base64()), BIO_new(BIO_s_mem())), free);
  if (!multiline) {
    BIO_set_flags(&*bio, BIO_FLAGS_BASE64_NO_NL);
  }

  BIO_write(&*bio, source, static_cast<int>(sourceLen));
  BIO_flush(&*bio);

  const BUF_MEM *buffer;
  BIO_get_mem_ptr(&*bio, &buffer);
  return {buffer->data, buffer->length};
}

std::vector<unsigned char> Base64::Decode(const std::string &source) {
  size_t resultLen = source.size();
  if (resultLen >= 2) {
    size_t padding = 0;
    if (source[resultLen - 1] == '=' && source[resultLen - 2] == '=') {
      padding = 2;
    } else if (source[resultLen - 1] == '=') {
      padding = 1;
    }
    resultLen = (resultLen * 3) / 4 - padding;
  } else {
    resultLen = (resultLen * 3) / 4;
  }
  std::vector<unsigned char> result(resultLen);

  const auto &free = [](BIO *bio) { BIO_free_all(bio); };
  const std::unique_ptr<BIO, decltype(free)> bio(
      BIO_push(BIO_new(BIO_f_base64()),
               BIO_new_mem_buf(const_cast<char *>(source.c_str()), -1)),
      free);
  BIO_set_flags(&*bio, BIO_FLAGS_BASE64_NO_NL);
  Verify(resultLen == static_cast<size_t>(BIO_read(
                          &*bio, &result[0], static_cast<int>(source.size()))));
  return result;
}

////////////////////////////////////////////////////////////////////////////////

std::string Crypto::EncodeToHex(const unsigned char *source, size_t sourceLen) {
  std::stringstream result;
  result << std::hex;
  for (size_t i = 0; i < sourceLen; ++i) {
    result << std::setw(2) << std::setfill('0')
           << static_cast<int>(*(source + i));
  }
  return result.str();
}

////////////////////////////////////////////////////////////////////////////////

boost::array<unsigned char, SHA512_DIGEST_LENGTH> Hmac::CalcSha512Digest(
    const unsigned char *source, size_t sourceLen, const std::string &key) {
  boost::array<unsigned char, SHA512_DIGEST_LENGTH> result;
  HMAC(EVP_sha512(), key.c_str(), static_cast<int>(key.size()), source,
       sourceLen, &result[0], nullptr);
  return result;
}

boost::array<unsigned char, SHA512_DIGEST_LENGTH> Hmac::CalcSha512Digest(
    const std::string &source, const std::string &key) {
  return CalcSha512Digest(
      reinterpret_cast<const unsigned char *>(source.c_str()), source.size(),
      key);
}

boost::array<unsigned char, SHA256_DIGEST_LENGTH> Hmac::CalcSha256Digest(
    const unsigned char *source, size_t sourceLen, const std::string &key) {
  return CalcSha256Digest(source, sourceLen,
                          reinterpret_cast<const unsigned char *>(key.c_str()),
                          key.size());
}
boost::array<unsigned char, SHA256_DIGEST_LENGTH> Hmac::CalcSha256Digest(
    const std::string &source, const std::string &key) {
  return CalcSha256Digest(
      reinterpret_cast<const unsigned char *>(source.c_str()), source.size(),
      key);
}
boost::array<unsigned char, SHA256_DIGEST_LENGTH> Hmac::CalcSha256Digest(
    const unsigned char *source,
    size_t sourceLen,
    const unsigned char *key,
    size_t keyLen) {
  boost::array<unsigned char, SHA256_DIGEST_LENGTH> result;
  HMAC(EVP_sha256(), key, static_cast<int>(keyLen), source, sourceLen,
       &result[0], nullptr);
  return result;
}
boost::array<unsigned char, SHA256_DIGEST_LENGTH> Hmac::CalcSha256Digest(
    const std::string &source, const unsigned char *key, size_t keyLen) {
  return CalcSha256Digest(
      reinterpret_cast<const unsigned char *>(source.c_str()), source.size(),
      key, keyLen);
}

////////////////////////////////////////////////////////////////////////////////

boost::array<unsigned char, MD5_DIGEST_LENGTH> Crypto::CalcMd5Digest(
    const std::string &source) {
  boost::array<unsigned char, MD5_DIGEST_LENGTH> result;
  MD5(reinterpret_cast<const unsigned char *>(source.c_str()), source.size(),
      &result[0]);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
