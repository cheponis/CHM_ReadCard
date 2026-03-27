// C.H --- UART Register definitions for PC UART

const int TRANSMIT_HOLDING_REGISTER           = 0x00;

const int RECEIVE_BUFFER_REGISTER             = 0x00;

const int INTERRUPT_ENABLE_REGISTER           = 0x01;
const int   IER_RX_DATA_READY                 = 0x01;
const int   IER_TX_HOLDING_REGISTER_EMPTY     = 0x02;
const int   IER_LINE_STATUS                   = 0x04;
const int   IER_MODEM_STATUS                  = 0x08;

const int INTERRUPT_ID_REGISTER               = 0x02;
const int   IIR_MODEM_STATUS_INTERRUPT        = 0x00;
const int   IIR_TX_HOLDING_REGISTER_INTERRUPT = 0x02;
const int   IIR_RX_DATA_READY_INTERRUPT       = 0x04;
const int   IIR_LINE_STATUS_INTERRUPT         = 0x06;

const int FIFO_CONTROL_REGISTER               = 0x02;

const int LINE_CONTROL_REGISTER               = 0x03;
const int   LCR_WORD_LENGTH_MASK              = 0x03;
const int   LCR_WORD_LENGTH_SELECT_0          = 0x01;
const int   LCR_WORD_LENGTH_SELECT_1          = 0x02;
const int   LCR_STOP_BITS                     = 0x04;
const int   LCR_PARITY_MASK                   = 0x38;
const int   LCR_PARITY_ENABLE                 = 0x08;
const int   LCR_EVEN_PARITY_SELECT            = 0x10;
const int   LCR_STICK_PARITY                  = 0x20;
const int   LCR_SET_BREAK                     = 0x40;
const int   LCR_DLAB                          = 0x80;

const int MODEM_CONTROL_REGISTER              = 0x04;
const int   MCR_DTR                           = 0x01;
const int   MCR_RTS                           = 0x02;
const int   MCR_OUT1                          = 0x04;
const int   MCR_OUT2                          = 0x08;
const int   MCR_LOOPBACK                      = 0x10;

const int LINE_STATUS_REGISTER                = 0x05;
const int   LSR_DATA_READY                    = 0x01;
const int   LSR_OVERRUN_ERROR                 = 0x02;
const int   LSR_PARITY_ERROR                  = 0x04;
const int   LSR_FRAMING_ERROR                 = 0x08;
const int   LSR_BREAK_DETECT                  = 0x10;
const int   LSR_TRANSMITTER_HOLDING_REGISTER_EMPTY = 0x20;
const int   LSR_TRANSMITTER_SHIFT_REGISTER_EMPTY = 0x40;

const int MODEM_STATUS_REGISTER               = 0x06;
const int   MSR_DELTA_CTS                     = 0x01;
const int   MSR_DELTA_DSR                     = 0x02;
const int   MSR_TERI                          = 0x04;
const int   MSR_DELTA_CD                      = 0x08;
const int   MSR_CTS                           = 0x10;
const int   MSR_DSR                           = 0x20;
const int   MSR_RI                            = 0x40;
const int   MSR_CD                            = 0x80;

const int DIVISOR_LATCH_LOW                   = 0x00;

const int DIVISOR_LATCH_HIGH                  = 0x01;

// End C.H
