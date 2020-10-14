#ifndef _Decode_
#define _Decode_

#include "../TFRSUnpackEvent.h"

// #define FILL_TGT_VUPROM
// #define FILL_TGT_V1742
// #define FILL_TGT_V0792
#define FILL_TGT_V1290

class Decode{
 public:
  Decode(TFRSUnpackEvent *tgt_, Int_t *pdata_);
  ~Decode(){};
  TFRSUnpackEvent *tgt;
  Int_t *pdata;
  Int_t const *pdata_first;
  void ReadCrateHeader();
  Int_t VME_SUB_SYS_ID = 0x00000200;
  Int_t HEADER_S4CRATE = 0x01aa0000;

  Int_t num_word_S4crate;
  
  void ReadVupromScaler();
  Int_t num_scaler_ch = 32;

  void ReadAllModules();
  Int_t num_reading_module; // defined in constructor;
  
private:
  void ReadEachModule();
  Int_t MODULE_TYPE_HEADER_MASK = 0x0000FFFF;
  Int_t PREFIX_TYPE_HEADER   = 0x02aa0000;
  Int_t PREFIX_MODULE_HEADER = 0x03aa0000;
  std::map<Int_t, void(Decode::*)()> pReadModuleFunc;
  //void (Decode::*pReadModuleFunc)();
  void FooterCheck();
  Int_t PREFIX_MODULE_TYPE_FOOTER = 0x02bb0000;
  Int_t PREFIX_MODULE_FOOTER      = 0x03bb0000;
  
  void DecodeV792();
  void DecodeV1290();
  void DecodeV1742();
  void V1742_HeaderCheck();
  void V1742_DataTaking(Int_t &group);
  const Int_t SAMPLING_NUM = 1024;
  const Int_t NUM_GROUP_ENABLE = 1;

  Int_t header_type;
  Int_t num_word_of_type;
  Int_t num_module;
  Int_t ntry_type;
  Int_t header_module;
  Int_t num_word_of_module;
  Int_t ith_module;
  Int_t ntry_module;
};

#endif
