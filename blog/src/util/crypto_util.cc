#include "crypto_util.h"
#include "sylar/util.h"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

namespace blog_server {
namespace util {

std::string CryptoUtil::md5(const std::string& str) {
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, str.c_str(), str.length());
    
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &ctx);
    
    char md5_str[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_str[i * 2], "%02x", (unsigned int)digest[i]);
    }
    md5_str[MD5_DIGEST_LENGTH * 2] = '\0';
    
    return std::string(md5_str);
}

std::string CryptoUtil::sha1(const std::string& str) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, str.c_str(), str.length());
    
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1_Final(digest, &ctx);
    
    char sha1_str[SHA_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&sha1_str[i * 2], "%02x", (unsigned int)digest[i]);
    }
    sha1_str[SHA_DIGEST_LENGTH * 2] = '\0';
    
    return std::string(sha1_str);
}

std::string CryptoUtil::base64Encode(const std::string& str) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    
    BIO_write(b64, str.c_str(), str.length());
    BIO_flush(b64);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    std::string result(bptr->data, bptr->length - 1); // 去掉换行符
    BIO_free_all(b64);
    
    return result;
}

std::string CryptoUtil::base64Decode(const std::string& str) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new_mem_buf(str.c_str(), str.length());
    bmem = BIO_push(b64, bmem);
    
    std::string result(str.length(), '\0');
    int len = BIO_read(bmem, &result[0], result.length());
    result.resize(len);
    
    BIO_free_all(bmem);
    return result;
}

} // namespace util
} // namespace blog_server