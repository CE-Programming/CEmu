#ifndef _H_EMU
#define _H_EMU

#include <string>


// Returns string of processed instruction
// start_addr: pointer to address in memory to try and stringify
// increment_amt: Pass by reference byte to tell how many bytes wide the instruction is
std::string processInstruction(uint8_t *start_addr, uint8_t &increment_amt);

#endif
