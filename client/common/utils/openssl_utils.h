#pragma once

#include <openssl/bio.h>
#include <openssl/evp.h>

namespace wsl
{

//---------------------------------------------------------------------------
class EvpPkeyCtx
{
public:
   EvpPkeyCtx(EVP_PKEY_CTX* pCtx);
   ~EvpPkeyCtx(void);

   bool isValid(void) const { return m_pCtx != NULL; }
   EVP_PKEY_CTX* context(void) { return m_pCtx; }

private:
   EVP_PKEY_CTX* m_pCtx;
};

inline
EvpPkeyCtx::EvpPkeyCtx(EVP_PKEY_CTX* pCtx)
   : m_pCtx(pCtx)
{
}

inline
EvpPkeyCtx::~EvpPkeyCtx(void)
{
   if (isValid()) {
      EVP_PKEY_CTX_free(m_pCtx);
   }
}

//---------------------------------------------------------------------------
class EvpPkey
{
public:
   EvpPkey(void);
   ~EvpPkey(void);

   bool isValid(void) const { return m_pKey != NULL; }
   EVP_PKEY* pkey(void) { return m_pKey; }
   EVP_PKEY** ppkey(void) { return &m_pKey; }

private:
   EVP_PKEY* m_pKey;
};

inline
EvpPkey::EvpPkey(void)
{
   m_pKey = NULL;
}

inline
EvpPkey::~EvpPkey(void)
{
   if (isValid()) {
      EVP_PKEY_free(m_pKey);
   }
}

//---------------------------------------------------------------------------
class EvpBioCharBuf
{
public:
   EvpBioCharBuf(void);
   ~EvpBioCharBuf(void);

   bool isValid(void) const { return pBIO_ != NULL; }
   BIO* getBIO() const { return pBIO_; }
   int write(const void *data, int dlen);

private:
   BIO* pBIO_;
};

inline
EvpBioCharBuf::EvpBioCharBuf(void)
{
   pBIO_ = BIO_new(BIO_s_mem());
}

inline
EvpBioCharBuf::~EvpBioCharBuf(void)
{
   if (pBIO_ != NULL) {
      BIO_free_all(pBIO_);
   }
}

inline
int EvpBioCharBuf::write(const void *data, int dlen)
{
    if (pBIO_ != NULL) {
        return BIO_write(pBIO_, data, dlen);
    }

    return 0;
}

}
