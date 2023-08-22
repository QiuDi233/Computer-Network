#include "switch.h"

SwitchBase* CreateSwitchObject() {
  // TODO : Your code.
  //return nullptr;
  return new EthernetSwitch();
}