#include "firmware.hpp"
#include <openssl/evp.h>
#include <memory>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
namespace Firmware {
struct Record { const char *name; size_t size; const char *hash; };
static const Record records[] = {
    {"MB86H57_H58_IDLE", 234912, "49c4b2fc9c1d1be8206cca4b356c13b3ded640ea328fdbd464a6aab279cd47f2"},
    {"MB86H57_H58_ENC_H", 649648, "c38b004ae7be1cbd956b8a1e11d3341d3d2df2bd3f92460538a37252dbfffc6b"},
    {"MB86M01_ASSP_NSEC_IDLE", 331600, "7bdf2e74d347957e818a3ecd15e103dd1e52baab5cdfe11acb3bda400a685280"},
    {"MB86M01_ASSP_NSEC_ENC_H", 1493072, "193871337a3e7a208db814d6354457a43da56f53e06f696fa025873b8c75e553"},
    {"MB86H57_H58_IDLE", 234912, "49c4b2fc9c1d1be8206cca4b356c13b3ded640ea328fdbd464a6aab279cd47f2"},
    {"MB86H57_H58_ENC_H", 649648, "c38b004ae7be1cbd956b8a1e11d3341d3d2df2bd3f92460538a37252dbfffc6b"},
    {"MB86M01_ASSP_NSEC_IDLE", 331696, "7fa65b482f995b4ba23fbe72b1b49feab764fbd75874e2438d704db3b8f34df0"},
    {"MB86M01_ASSP_NSEC_ENC_H", 1500784, "22d9d9b4e5a1a3b6ec3953dacaa2d17ab4cf157ab08c308f07afc0346388393b"},
};
void validate(FILE *file, const std::string &path) {
    std::string name=path.substr(path.find_last_of('/')+1);
    std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){ return std::toupper(c); });
    if (name.size()>4 && name.substr(name.size()-4)==".BIN") name.resize(name.size()-4);
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx || EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)!=1) throw std::runtime_error("Cannot initialize firmware SHA-256");
    unsigned char block[16384]; size_t total=0, count;
    while ((count=fread(block,1,sizeof block,file))!=0) {
        total+=count;
        if (total>16*1024*1024 || EVP_DigestUpdate(ctx.get(),block,count)!=1) throw std::runtime_error("Firmware digest failed");
    }
    if (ferror(file)) throw std::runtime_error("Firmware read failed during verification");
    unsigned char digest[EVP_MAX_MD_SIZE]; unsigned length=0;
    if (EVP_DigestFinal_ex(ctx.get(),digest,&length)!=1) throw std::runtime_error("Firmware digest failed");
    std::ostringstream hash;
    for (unsigned i=0;i<length;++i) hash << std::hex << std::setfill('0') << std::setw(2) << unsigned(digest[i]);
    bool known=false;
    for (const auto &record: records) if (name==record.name && total==record.size && hash.str()==record.hash) known=true;
    if (!known) throw std::runtime_error("Firmware size/SHA-256 is not a verified official version: "+path);
    if (fseek(file,0,SEEK_SET)!=0) throw std::runtime_error("Cannot rewind verified firmware");
}
}
