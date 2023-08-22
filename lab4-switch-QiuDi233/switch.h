#ifndef COMPNET_LAB4_SRC_SWITCH_H
#define COMPNET_LAB4_SRC_SWITCH_H

#include "types.h"
#include<map>
class SwitchBase {
 public:
  SwitchBase() = default;
  ~SwitchBase() = default;

  virtual void InitSwitch(int numPorts) = 0;
  virtual int ProcessFrame(int inPort, char* framePtr) = 0;
};

extern SwitchBase* CreateSwitchObject();

// TODO : Implement your switch class.
struct table{
  int port;
  int count;
};
uint64_t mac2int64(mac_addr_t mac){
  uint64_t result=0;
  memcpy(&result,mac,6);
  return result;
}
class EthernetSwitch : public SwitchBase {
public:
  int port_num=0;
  std::map<uint64_t,table>fwd;//forwarding table
  
  void InitSwitch(int numPorts) {
    port_num=numPorts;
  }
  int ProcessFrame(int inPort, char* framePtr){
    if(((ether_header_t*)framePtr)->ether_type==ETHER_CONTROL_TYPE){
        //AGING
        for (auto it = fwd.begin(); it != fwd.end(); ) {
            ((it->second).count)--;
            if(((it->second).count)==0){
              it=fwd.erase(it);
            }
            else{
              it++;
            }
        }
        return -1;
    }

    if(((ether_header_t*)framePtr)->ether_type==ETHER_DATA_TYPE){
      
      uint64_t src=mac2int64(((ether_header_t*)framePtr)->ether_src);
      uint64_t dest=mac2int64(((ether_header_t*)framePtr)->ether_dest);

      if(fwd.find(src)!=fwd.end()){
        fwd[src].count=ETHER_MAC_AGING_THRESHOLD;//刷新
      }
      else{
        table tmp=table{inPort,ETHER_MAC_AGING_THRESHOLD};
        fwd.insert(std::make_pair(src,tmp));
      }
      
      if(fwd.find(dest)==fwd.end()){//not found
        //boardcast
        return 0;
      }
      else{
        if(fwd[dest].port==inPort){
          return -1;//discard
        }
        else{
          return fwd[dest].port;
        }
      }
    }
  }
};
#endif  // ! COMPNET_LAB4_SRC_SWITCH_H
