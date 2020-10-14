#ifndef _DecodeCXX_
#define _DecodeCXX_

#include "Decode.h"
//#define PRINT_DEBUG
//#define PRINT_BATCH

Decode::Decode(TFRSUnpackEvent *tgt_, Int_t *pdata_){
  tgt = tgt_;
  pdata = pdata_;
  pdata_first = pdata_;
  pReadModuleFunc[0x0792] = &Decode::DecodeV792;
  pReadModuleFunc[0x1290] = &Decode::DecodeV1290;
  pReadModuleFunc[0x1742] = &Decode::DecodeV1742;
  num_reading_module = (Int_t)pReadModuleFunc.size();
}

void Decode::ReadCrateHeader(){
  Int_t vme_sub_sys_id  = *pdata++;
  Int_t header_s4_crate = *pdata++;
  if(VME_SUB_SYS_ID != vme_sub_sys_id){
    std::cout << "First word is wrong" <<  std::endl;
  }
  if(HEADER_S4CRATE != header_s4_crate){
    std::cout << "S4 crate header wrong" << std::endl;
  }

  num_word_S4crate = *pdata++;
  
#ifdef PRINT_DEBUG
  std::cout << "num_word --> "  << std::dec << num_word_S4crate << std::endl;
  std::cout << "Header OK" << std::endl;
#endif
  
}

void Decode::ReadVupromScaler(){
  for(Int_t i = 0 ; i < num_scaler_ch ; i++){
    Int_t idata        = *pdata++;
    Int_t ch           = 0xFF & (idata >> 24);
    Int_t scaler_value = 0xFFFFF & idata;

#ifdef FILL_TGT_VUPROM
    
    if(ch < num_scaler_ch){
      tgt -> vme2scaler[ch] = scaler_value;
    }

#endif
    
#ifdef PRINT_DEBUG
    std::cout << "ch : " << ch
	      << " scaler_value --> " << scaler_value<< std::endl;
#endif
  }
  
#ifdef PRINT_DEBUG
  std::cout << "Scaler OK" << std::endl;
#endif
  
}

void Decode::ReadAllModules(){
  
  for(Int_t i = 0 ; i < num_reading_module ;i++){
    ReadEachModule();
  }

}

void Decode::ReadEachModule(){
#ifdef PRINT_BATCH
  std::cout << " -- Reading each module -- " << std::endl;
#endif
  header_type        = *pdata++;
  num_word_of_type   = *pdata++;
  num_module         = *pdata++;
  ntry_type          = *pdata++;
  
#ifdef PRINT_DEBUG
  std::cout << "Header type : " << std::hex << header_type << std::endl;
  std::cout << "Word num : "    << std::dec << num_word_of_type << std::endl;
  std::cout << "Module num : "  << std::dec << num_module << std::endl;
  std::cout << "ntry type ? : " << std::hex << ntry_type << std::endl;
#endif
  if(PREFIX_TYPE_HEADER != (0xFFFF0000 & header_type)){
    std::cout << "S4 module type header wrong !! " << std::endl;
  }
  Int_t which_module = MODULE_TYPE_HEADER_MASK & header_type;
  
  // ---- Check each module header ---- //
  header_module      = *pdata++;
  num_word_of_module = *pdata++;
  ith_module         = *pdata++;
  ntry_module        = *pdata++;
  
#ifdef PRINT_DEBUG
  std::cout << "Module type       : " << std::hex << header_module << std::endl;
  std::cout << "Word num (module ): " << std::dec << num_word_of_module << std::endl;
  std::cout << "ith module        : "   << std::dec << ith_module << std::endl;
  std::cout << "ntry module ?     : " << std::hex << ntry_module << std::endl;
#endif

  if(PREFIX_MODULE_HEADER != (0xFFFF0000 & header_module)){
    std::cout << "S4 module header wrong !! ("
	      << std::hex << header_module << std::endl;
  }else{
#ifdef PRINT_BATCH
    std::cout << "// module header OK" << std::endl;
#endif
  }
  if(pReadModuleFunc[which_module]){
    (this ->*pReadModuleFunc[which_module])();
  }else{
    std::cout << "S4 module type is wrong " <<  std::endl;
  }
}

// ------------------- V792 -------------------- //
void Decode::DecodeV792(){
#ifdef PRINT_BATCH
  std::cout << "-- Decoding V792 -- " << std::endl;
#endif
  Int_t length = num_word_of_module;
  Int_t itry   = ntry_module;
  pdata++;
  
  // read 32 words changed for v792
  for(Int_t i = 0 ; i < 32 ; i++){
    Int_t idata = *pdata++;
    Int_t v792_type = 0x7 & ( idata >> 24 );
    Int_t v792_ch, v792_value, v792_un, v792_ov;
    if(v792_type == 0){
      v792_ch    = 0x1F  & (idata >> 16);
      v792_value = 0xFFF & idata;
      v792_un    = 0x1   & (idata >> 13);
      v792_ov    = 0x1   & (idata >> 12);
      
#ifdef FILL_TGT_V0792
      
      if(v792_ov == 0 && v792_un == 0){
	tgt -> v792q[ith_module][v792_ch] = v792_value;
      }else if(v792_un == 0){
	tgt -> v792q[ith_module][v792_ch] = 99999;
      }else{
	tgt -> v792q[ith_module][v792_ch] = -99999;
      }//end of if
      
#endif
      
    }// end of if
#ifdef PRINT_DEBUG
    std::cout << std::dec << "ch : " << v792_ch << ", value : " << v792_value
	      << " , un : " << v792_un << ", ov : " << v792_ov << std::endl;
    
#endif
  }// end of <i>
  // ---- skip last word ----
#ifdef PRINT_BATCH
  std::cout << "Now : " << pdata - pdata_first << " : value --> "
	    << std::hex <<*pdata << std::endl;
#endif
  pdata++;
  FooterCheck();


}

// ---------------------- V1290 ---------------- //

void Decode::DecodeV1290(){
#ifdef PRINT_BATCH
  std::cout << "-- Decoding V1290 --" << std::endl;
#endif
  
#ifdef PRINT_DEBUG
  std::cout << "num of word : " << std::dec << num_word_of_module << std::endl;
#endif
  Int_t length   = num_module;
  Int_t ntry_mod = ntry_module;
    
  std::vector<int> hit_counter_leading(32, 0);
  std::vector<int> hit_counter_trailing(32, 0);
  for(int i = 0 ; i < num_word_of_module - 5 ; i++){
    Int_t idata = *pdata++;
    Int_t v1290_type = 0x1F & ( idata >> 27);

    if(v1290_type == 0){// TDC measurement
      Int_t v1290_edge  = 0x1  & (idata >> 26);
      Int_t v1290_ch    = 0x1F & (idata >> 21);
      Int_t v1290_value = 0x1FFFFF & idata;

      // if(v1290_edge == 0){
      // std::cout << "edge  : " << v1290_edge  << ", "
      // 		<< "ch : " << v1290_ch    << ", "
      // 		<< "value : " << std::dec << v1290_value << std::endl;
      // }
#ifdef FILL_TGT_V1290
      if(v1290_edge == 0){
	int ihit = hit_counter_leading[v1290_ch]++;
	tgt -> v1290l_multi[ith_module][v1290_ch][ihit] = v1290_value;
#ifdef PRINT_DEBUG
	std::cout << "Leading "
		  << "ith module : " << ith_module << " "
		  << "ch : " << v1290_ch << " "
		  << "ihit : " << ihit << " "
		  << "val : " << v1290_value << std::endl;
#endif
      }else if(v1290_edge == 1){
	int ihit = hit_counter_trailing[v1290_ch]++;
	tgt -> v1290t_multi[ith_module][v1290_ch][ihit] = v1290_value;
#ifdef PRINT_DEBUG
	std::cout << "Trailing "
		  << "ith module : " << ith_module << " "
		  << "ch : " << v1290_ch << " "
		  << "ihit : " << ihit << " "
		  << "val : " << v1290_value << std::endl;
#endif
      }
#endif
      
    }else if(v1290_type == 8){// global header
      Int_t event_count = (idata >> 5) & 0x3FFFFF;
      Int_t geo = idata & 0x1F;
      // std::cout << " v1290 type  " << v1290_type
      // 		<< " event_count : " << event_count
      // 		<< " geo : " << geo << std::endl;
    }else if(v1290_type == 1){// TDC header
      Int_t TDC = (idata >> 24) & 0x3;
      Int_t event_id = (idata >> 12) & 0xFFF;
      Int_t bunch_id = idata & 0xFFF;
      // std::cout << " v1290 type  " << v1290_type
      // 		<< " TDC : " << TDC
      // 		<< " event_id : " << event_id
      // 		<< " bunch_id : " << bunch_id << std::endl;
    }else if(v1290_type == 3){// TDC Trailer
      Int_t word_count = idata && 0xFFF;
      Int_t event_id   = (idata >> 12) & 0xFFF;
      Int_t TDC        = (idata >> 24) & 0x3;
      // std::cout << " v1290 type  " << v1290_type
      // 		<< " TDC : " << TDC
      // 		<< " event_id : " << event_id
      // 		<< " word_count : " << word_count << std::endl;
    }else if(v1290_type == 4){// 
      Int_t error_flags = idata & 0x3FFF;
      Int_t TDC         = (idata >> 24) & 0x3;
      // std::cout << " v1290 type  " << v1290_type
      // 		<< " TDC : " << TDC
      // 		<< " Error flags : " << error_flags << std::endl;
    }else{
      // std::cout << " v1290 type  " << v1290_type << std::endl;
    }
  }

  FooterCheck();
}

// ------------------- V1742 ----------------------------- //
////
//// Somehow in the header, there are a few lines of words "0x2e2e2e2e"
//// between the first line and the second line of the header.
//// This is not written in the manual of V1742. I guess this word
//// was put by hand by someone.

void Decode::DecodeV1742(){

#ifdef PRINT_BATCH
  std::cout << "-- Decoding V1742 -- " << std::endl;
#endif

  Int_t length = num_module;
  Int_t itry   = ntry_module;
  
  V1742_HeaderCheck();
  
  for(Int_t i = 0 ; i <= NUM_GROUP_ENABLE ; i++){
    V1742_DataTaking( i );
  }

  FooterCheck();
}

void Decode::V1742_HeaderCheck(){
  Int_t header[4];
  header[0] = *pdata++;

  Int_t v1742header = 0xF & (header[0] >> 28);
  Int_t event_size  = 0xFFFFFFF & header[0];
  if(0xA != v1742header){
    std::cout << "V1742 Event Header Wrong !!" << std::endl;
  }
  // std::cout << " check " << std::hex << *pdata << std::endl;
  while(*pdata == 0x2e2e2e2e){
    ++pdata;
  }
  header[1] = *pdata++;
  header[2] = *pdata++;
  header[3] = *pdata++;
  Int_t board_id       = header[1] >> 27;
  Int_t event_counter  = header[2] & 0xFFFFFF;
  Int_t event_time_tag = header[3];
#ifdef PRINT_BATCH  
  std::cout << "Now : " << std::dec << pdata - pdata_first << " "
	    << std::hex << *pdata << std::endl;
#endif


}

void Decode::V1742_DataTaking( Int_t &group ){
  Int_t group_event_description = *pdata++;
  Int_t start_index_cell = (group_event_description >> 20) & 0x3FF;
  Int_t TR   = 0xF & group_event_description >> 12;
  Int_t SIZE = 0xFFF & group_event_description;
  Int_t frequency = (group_event_description >> 16) & 0x3;
#ifdef PRINT_DEBUG
  std::cout << "Freq : " << frequency << std::endl;
  std::cout << "Star Index Cell : " << start_index_cell
	    << "  TR : " << TR << "  SIZE :" << SIZE << std::endl;
  std::cout << "Now : " << std::hex << *pdata << " "
	    << std::dec << pdata - pdata_first <<  std::endl;
#endif
  for(Int_t i = 0 ; i < SAMPLING_NUM ; i++){
    Int_t buf0 = *pdata++;
    Int_t buf1 = *pdata++;
    Int_t buf2 = *pdata++;
    Int_t data[8]; // for 1 group
    data[0] = buf0 & 0xFFF;
    data[1] = (buf0 >> 12) & 0xFFF;
    data[2] = (buf0 >> 24) & 0x0FF + (buf1 << 8) & 0xF00;
    data[3] = (buf1 >> 4 ) & 0xFFF;
    data[4] = (buf1 >> 16) & 0xFFF;
    data[5] = (buf1 >> 28) & 0x00F + (buf2 << 4) & 0xFF0;
    data[6] = (buf2 >> 8 ) & 0xFFF;
    data[7] = (buf2 >> 20) & 0xFFF;

#ifdef FILL_TGT_V1742
    // std::cout << i << " ";
    for(Int_t ch = 0 ; ch < 8 ; ch++){
      tgt -> v1742[ch + 8 * group][i] = data[ch];
      // std::cout << std::dec << data[ch] << "  ";
    }// end of <ch>
    // std::cout << std::endl;
#endif
  }// end of <i>
  if(TR != 0){
    for(Int_t i = 0 ; i < SAMPLING_NUM ; i++){
      pdata++;
    }// end of <i>
  }
  
  Int_t group_trigger_time_tag = *pdata++;
#ifdef PRINT_DEBUG
  std::cout << "Now : " << std::hex << *pdata << " "
	    << std::dec << pdata - pdata_first << std::endl;
#endif
  
}

// ------------------ FOOTER CHECK ------------------ //

void Decode::FooterCheck(){
  
  Int_t footer_module      = *pdata++;
  Int_t footer_type_module = *pdata++;
  if(footer_module & 0xFFFF0000 != PREFIX_MODULE_FOOTER){
    std::cout << "Module Footer Wrong !! " << std::endl;
    std::cout << "Now : " << std::dec << pdata - pdata_first
	      << " " << std::hex << *pdata << std::endl;
  }
  if(footer_type_module & 0xFFFF0000 != PREFIX_MODULE_TYPE_FOOTER){
    std::cout << "Module TYPE Footer Wrong !! " << std::endl;
    std::cout << "Now : " << std::dec << pdata - pdata_first
	      << " "  << std::hex << *pdata <<std::endl;
  }
  //std::cout << std::hex << (footer_type_module & 0xFFFF0000) << std::endl;
  assert( (footer_type_module & 0xFFFF0000) == PREFIX_MODULE_TYPE_FOOTER);
  
#ifdef PRINT_DEBUG  
  std::cout << "Now : " << *pdata << " distance : " << pdata - pdata_first << std::endl;
#endif

#ifdef PRINT_BATCH
  std::cout << "---- Footer check OK. Now " << std::dec << pdata - pdata_first<< std::endl;
#endif

}
#endif
