from intelhex import IntelHex
import serial
import time

"""
Each line of Intel HEX file consists of six parts:

    Start code, one character, an ASCII colon ':'.
    Byte count, two hex digits, a number of bytes (hex digit pairs) in the data field. 16 (0x10) or 32 (0x20) bytes of data are the usual compromise values between line length and address overhead.
    Address, four hex digits, a 16-bit address of the beginning of the memory position for the data. Limited to 64 kilobytes, the limit is worked around by specifying higher bits via additional record types. This address is big endian.
    Record type, two hex digits, 00 to 05, defining the type of the data field.
    Data, a sequence of n bytes of the data themselves, represented by 2n hex digits.
    Checksum, two hex digits - the least significant byte of the two's complement of the sum of the values of all fields except fields 1 and 6 (Start code ":" byte and two hex digits of the Checksum). It is calculated by adding together the hex-encoded bytes (hex digit pairs), then leaving only the least significant byte of the result, and making a 2's complement (either by subtracting the byte from 0x100, or inverting it by XOR-ing with 0xFF and adding 0x01). If you are not working with 8-bit variables, you must suppress the overflow by AND-ing the result with 0xFF. The overflow may occur since both 0x100-0 and (0x00 XOR 0xFF)+1 equal 0x100. If the checksum is correctly calculated, adding all the bytes (the Byte count, both bytes in Address, the Record type, each Data byte and the Checksum) together will always result in a value wherein the least significant byte is zero (0x00).
    For example, on :0300300002337A1E
    03 + 00 + 30 + 00 + 02 + 33 + 7A = E2, 2's complement is 1E

There are six record types:

    00, data record, contains data and 16-bit address. The format described above.
    01, End Of File record. Must occur exactly once per file in the last line of the file. The byte count is 00 and the data field is empty. Usually the address field is also 0000, in which case the complete line is ':00000001FF'. Originally the End Of File record could contain a start address for the program being loaded, e.g. :00AB2F0125 would cause a jump to address AB2F. This was convenient when programs were loaded from punched paper tape.
    02, Extended Segment Address Record, segment-base address (two hex digit pairs in big endian order). Used when 16 bits are not enough, identical to 80x86 real mode addressing. The address specified by the data field of the most recent 02 record is multiplied by 16 (shifted 4 bits left) and added to the subsequent data record addresses. This allows addressing of up to a megabyte of address space. The address field of this record has to be 0000, the byte count is 02 (the segment is 16-bit). The least significant hex digit of the segment address is always 0.
    03, Start Segment Address Record. For 80x86 processors, it specifies the initial content of the CS:IP registers. The address field is 0000, the byte count is 04, the first two bytes are the CS value, the latter two are the IP value.
    04, Extended Linear Address Record, allowing for fully 32 bit addressing (up to 4GiB). The address field is 0000, the byte count is 02. The two data bytes (two hex digit pairs in big endian order) represent the upper 16 bits of the 32 bit address for all subsequent 00 type records until the next 04 type record comes. If there is not a 04 type record, the upper 16 bits default to 0000. To get the absolute address for subsequent 00 type records, the address specified by the data field of the most recent 04 record is added to the 00 record addresses.
    05, Start Linear Address Record. The address field is 0000, the byte count is 04. The 4 data bytes represent the 32-bit value loaded into the EIP register of the 80386 and higher CPU.

"""

class PhyDownloader:

    def __init__(self, serialName, hexfile):
        self.hex = IntelHex(hexfile)
        self.serial = serial.Serial(serialName, 115200, timeout=5)
        self.write_index = 0

    def reset(self, boot=True):
        # reset = LOW, TM = HIGH
        self.serial.dtr = False
        self.serial.rts = True
        time.sleep(0.1)
        
        # reset = HIGH, TM = HIGH
        if boot == True:
            self.serial.rts = False
        # reset = HIGH, TM = LOW
        else:
            self.serial.rts = False
            self.serial.dtr = True
        
        time.sleep(0.1)
    
    def change_baudrate(self, baudrate):
        if baudrate > 2000000 or baudrate < 9600:
            raise Exception("baudrate must be in range [9600, 2000000]")
        self.serial.flushInput()
        cmd = "uarts{0}".format(baudrate)
        self.serial.write(cmd.encode())
        print('>: ' + cmd)

        time.sleep(0.2)
        # self.serial._set_special_baudrate(baudrate)
        # self.serial.baudrate = baudrate
        # self.serial.flushInput()
        # print(self.serial.baudrate)

        self.serial.baudrate = baudrate
        self.serial.flushInput()

        ret = self.serial.read_until(b'#OK>>:')
        print(ret)
        print('<: ' + ret.decode())

        if ret.endswith(b'#OK>>:') == False:
            print('change baudrate failed, will use default baudrate(115200)')
        else:
            self.serial.baudrate = baudrate
            print('change baudrate successed, use baudrate:{0}'.format(baudrate))

    def erase_512k(self):
        self.serial.flushInput()
        # write commnad
        er512 = 'er512'
        self.serial.write(er512.encode())
        print('>: ' + er512)

        # read response
        ret = self.serial.read_until(b'#OK>>:')
        print('<: ' + ret.decode())

        # is successed?
        if ret.endswith(b'#OK>>:') == False:
            raise Exception("erase failed")
    
    def write_cpnum(self, num):
        self.serial.flushInput()

        # write command 
        # cpnum = 'cpnum {0}'.format(num)       // 无效？？？
        cpnum = 'cpnum ffffffff'
        self.serial.write(cpnum.encode())
        print('>: ' + cpnum)

        # read response 
        ret = self.serial.read_until(b'#OK>>:')
        print('<: ' + ret.decode())

        # is successed?
        if ret.endswith(b'#OK>>:') == False:
            raise Exception("cpnum write failed")
            
    def write_bin(self, start_address, end_address):
        print('-------------------------------')
        # cpbin 指令
        #   cpbin [index] [flash address] [size] [run address]
        flash_address = start_address % 0x100000
        size = end_address - start_address
        cmd = 'cpbin c{0} {1:06x} {2:x} {3:x}'.format(self.write_index, flash_address, size, start_address)
        self.serial.flushInput()
        self.serial.write(cmd.encode())
        print('>: ' + cmd)

        # wait for response:'by hex mode'
        # ret = self.serial.read(len(expecting_response))
        ret = self.serial.read_until(b':')
        print('<: ' + ret.decode())

        # 是否成功进入hex模式?
        if ret != b'by hex mode:':
            raise Exception("device no response")

        # send [data]
        bin = self.hex.tobinarray(start_address, end_address-1)
        print('>: ' + 'upload {0:x} byte binary stub'.format(len(bin)))
        self.serial.write(bin)
        send_checksum = sum(bin)   # 求 bin 文件数据的字节累加和,累加和的数据类型为无符号整型四字节,不计进位

        # 等待返回校验值
        ret = self.serial.readline()    # wait for checksum:'checksum is 0x--------\n'
        print('<:' + ret.decode().strip('\n'))
        if ret == None:
            raise Exception('checksum no response')
        else:
            recv_checksum = int(ret[-8:], base=16)
            if recv_checksum != send_checksum:
                raise Exception('checksum mismatch')

        # send checksum 
        checksum_bytes = bytes('{:08x}'.format(send_checksum), encoding='utf-8')
        self.serial.write(checksum_bytes)
        print('>: ' + 'checksum:' + checksum_bytes.decode())

        # and wait for:'#OK>>:'
        ret = self.serial.read_until(b'#OK>>:')
        print('<: ' + ret.decode())

        # is OK?
        if ret != b'#OK>>:':
            raise Exception('checksum failed')
        print('send bin sucesssed\n')
        self.write_index = self.write_index + 1
 
hexFile = "generated/total_image.hex"

phy = PhyDownloader('/dev/ttyUSB0', hexFile)
# phy = PhyDownloader('COM11', hexFile)
phy.reset(boot=True)

phy.change_baudrate(500000)

phy.erase_512k()
phy.write_cpnum(10)

segments = phy.hex.segments()

for seg in segments:
    start = seg[0]
    end = seg[1]
    phy.write_bin(start, end)

phy.reset(boot=False)
phy.serial.close()

import os
os.system('python3 -m serial.tools.miniterm /dev/ttyUSB0 115200')