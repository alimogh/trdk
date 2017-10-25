/*******************************************************************************
 *   Created: 2017/10/10 09:12:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Lib {

class Base64Coder : private boost::noncopyable {
 public:
  explicit Base64Coder(bool multiline)
      : m_bio(BIO_push(BIO_new(BIO_f_base64()), BIO_new(BIO_s_mem()))) {
    if (!multiline) {
      BIO_set_flags(m_bio, BIO_FLAGS_BASE64_NO_NL);
    }
  }
  ~Base64Coder() {
    BIO_set_close(m_bio, BIO_NOCLOSE);
    BIO_free_all(m_bio);
  }

 public:
  void Encode(const unsigned char *source,
              size_t sourceLen,
              std::ostream &stream) {
    BIO_write(m_bio, source, static_cast<int>(sourceLen));
    BIO_flush(m_bio);

    const BUF_MEM *buffer;
    BIO_get_mem_ptr(m_bio, &buffer);
    stream.write(buffer->data, buffer->length);
  }

 private:
  BIO *const m_bio;
};
}
}