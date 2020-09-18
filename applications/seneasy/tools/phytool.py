import argparse
import sys,os
import serial
from downloader import PhyDownloader
import struct

def main():
    parse = argparse.ArgumentParser(prog="phy6212 tools")
    parse.add_argument("-p", "--port", dest="port")
    parse.add_argument("-t", "--terminal", dest="t", action="store_true")
    subparser = parse.add_subparsers(dest="subcommands", title="subcommands", description="支持的操作", help="select the task")
    # 子命令:flash
    parser_flash = subparser.add_parser("flash", help="下载固件")
    parser_flash.add_argument("-f", "--firmwave", dest="firmware")
    parser_flash.set_defaults(func=flash)
    # 子命令:MAC
    parse_mac = subparser.add_parser("MAC", help="写入mac地址")
    parse_mac.add_argument("-a", "--address", dest="address", help="XX-XX-XX-XX-XX-XX")
    parse_mac.set_defaults(func=write_mac)

    args = parse.parse_args()
    print(args)

    if (args.subcommands != None):
        args.func(args)

    if args.t == True:
        os.system('python3 -m serial.tools.miniterm ' + args.port + ' 768000 --rts 0 --dtr 1')

    # args.func()    

def flash(args): 
    # hexFile = "generated/total_image.hex"
    # phy = PhyDownloader('/dev/ttyUSB0', hexFile)
    phy = PhyDownloader(args.port, args.firmware)
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

def write_mac(args):
    try:
        mac = [int(x, base=16) for x in args.address.split('-')]
        if (len(mac) != 6):
            raise Exception("mac address format error")
        bytes_mac = struct.pack("BBBBBB", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5])
    except:
        raise Exception("mac address format error")

    phy = PhyDownloader(args.port, None)
    phy.reset(boot=True)
    # phy.write_mac(b'\x3B\x7A\xCB\x15\x00\x02')
    phy.write_mac(bytes_mac)
    phy.reset(boot=False)
    phy.serial.close()

if __name__ == "__main__":
    main()