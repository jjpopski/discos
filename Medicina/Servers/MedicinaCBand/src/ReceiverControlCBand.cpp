#include "ReceiverControlCBand.h"

ReceiverControlCBand::ReceiverControlCBand(
    const std::string dewar_ip,
    const unsigned short dewar_port, 
    const std::string lna_ip, 
    const unsigned short lna_port, 
    const unsigned int guard_time,
    const unsigned short number_of_feeds,
    const std::string switch_ip, 
    const unsigned short switch_port, 
    const BYTE dewar_madd,
    const BYTE dewar_sadd,
    const BYTE lna_madd,
    const BYTE lna_sadd,
    const BYTE switch_madd,
    const BYTE switch_sadd,
    bool reliable_comm) 
    throw (IRA::ReceiverControlEx) : 
    IRA::ReceiverControl(dewar_ip, dewar_port,
                        lna_ip, lna_port,
                        guard_time,
                        number_of_feeds,
                        switch_ip, switch_port,
                        dewar_madd, dewar_sadd,
                        lna_madd, lna_sadd,
                        switch_madd, switch_sadd,
                        reliable_comm)
{

}


void ReceiverControlCBand::setReceiverHigh(
        const BYTE data_type, 
        const BYTE port_type, 
        const BYTE port_number, 
        const BYTE value
        ) throw (IRA::ReceiverControlEx)
{
    try {
        makeRequest(
                m_dewar_board_ptr,     // Pointer to the dewar board
                MCB_CMD_SET_DATA,      // Command to send
                4,                     // Number of parameters
                data_type, 
                port_type,  
                port_number, 
                value 
        );
    }
    catch(IRA::MicroControllerBoardEx& ex) {
        std::string error_msg = "ReceiverControl: error performing setReceiverHigh.\n";
        throw IRA::ReceiverControlEx(error_msg + ex.what());
    }    
}

void ReceiverControlCBand::setReceiverLow(
        const BYTE data_type, 
        const BYTE port_type, 
        const BYTE port_number, 
        const BYTE value
        ) throw (IRA::ReceiverControlEx)
{
    try {
        makeRequest(
                m_dewar_board_ptr,     // Pointer to the dewar board
                MCB_CMD_SET_DATA,      // Command to send
                4,                     // Number of parameters
                data_type, 
                port_type,  
                port_number, 
                value 
        );
    }
    catch(IRA::MicroControllerBoardEx& ex) {
        std::string error_msg = "ReceiverControl: error performing setReceiverHigh.\n";
        throw IRA::ReceiverControlEx(error_msg + ex.what());
    }    
}