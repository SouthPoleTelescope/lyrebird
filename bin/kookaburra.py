#!/usr/bin/env python

import numpy as np
import socket, curses, json, traceback, math, argparse, math, sys, os, stat
from operator import itemgetter, attrgetter
from configutils.dfmux_config_constructor import get_physical_id, sq_phys_id_to_info
from configutils.dfmux_config_constructor import uniquifyList, generate_dfmux_lyrebird_config
#from spt3g.util import genericutils as GU # not in the public S4 repo
from spt3g import core, dfmux, calibration
from functools import cmp_to_key


import signal 
import warnings
warnings.filterwarnings("ignore")
def split_on_numbers(s):
    '''
    Splits the string into a list where the numbers and the characters between numbers are each element
    Copied from spt3g_software to fix dependencies (sorry)
    '''
    prevDig = False
    outList = []
    for char in s:
        if char.isdigit():
            if prevDig:
                outList[-1] += char
            else:
                prevDig = True
                outList.append(char)
        else:
            if not prevDig and len(outList)>0:
                outList[-1] += char
            else:
                prevDig = False
                outList.append(char)
            
    return outList
        
def str_cmp_with_numbers_sorted(str1, str2):
    '''
    Compares two strings where numbers are sorted according to value, so Sq12 ends up after Sq8,  use in sorted function
    Copied from spt3g_software to fix dependencies (sorry)
    '''

    if str1==str2:
        return 0
    split1 = split_on_numbers(str1)
    split2 = split_on_numbers(str2)

    largestStr = 0
    for l in [split1, split2]:
        for s in l:
            if s[0].isdigit():
                largestStr = len(s) if len(s) > largestStr else largestStr

    for l in [split1, split2]:
        for i in range(len(l)):
            if l[i][0].isdigit():
                l[i] =  '0'*(largestStr-len(l[i])) +l[i]

    p1 = reduce(lambda x,y: x+y, split1)
    p2 = reduce(lambda x,y: x+y, split2)
    return -1 if p1<p2 else 1


@core.cache_frame_data(type = core.G3FrameType.Housekeeping, wiring_map = 'WiringMap',
                       tf = 'DfMuxTransferFunction', system = 'ReadoutSystem')
def AddVbiasAndCurrentConv(frame, wiring_map):
    hk_map = frame['DfMuxHousekeeping']
    v_bias = core.G3MapDouble()
    i_conv = core.G3MapDouble()
    for k in wiring_map.keys():
        vb = dfmux.unittransforms.bolo_bias_voltage_rms(wiring_map, hk_map, 
                                                        bolo = k, tf = tf, system = system) / core.G3Units.V
        ic = dfmux.unittransforms.counts_to_rms_amps(wiring_map, hk_map, 
                                                     bolo = k, tf = tf, system = system) / core.G3Units.amp
        v_bias[k] = vb
        i_conv[k] = ic
    frame['VoltageBias'] = v_bias
    frame['CurrentConv'] = i_conv


def make_square_block(n_things):
    sq = n_things**0.5
    if n_things == int(math.floor(sq))**2:
        return (sq,sq)
    else:
        sq = int(math.floor(sq))
        return (sq, sq+1)

def write_get_hk_script(fn, hostname, port):
    script = '''#!/bin/bash
nc -w 1 %s %d
''' % (hostname, port)
    f = open(fn, 'w')
    f.write(script)
    f.close()
    st = os.stat(fn)
    os.chmod(fn, st.st_mode | stat.S_IXUSR)


class BoloPropertiesFaker(object):
    def __init__(self):
        self.wiring_map = None
        self.bolo_props = None
        self.sent_off = False
        self.default_tf = 'spt3g_filtering_2017_full'
        return

    def __call__(self, frame):
        if 'DfMuxTransferFunction' in frame:
            self.default_tf = frame['DfMuxTransferFunction']
        if frame.type == core.G3FrameType.Wiring:
            self.wiring_map = frame['WiringMap']
            return self.send_off(frame)
        elif frame.type == core.G3FrameType.Calibration:
            if 'BolometerProperties' in frame:
                self.bolo_props = frame['BolometerProperties']
            elif 'NominalBolometerProperties' in frame:
                self.bolo_props = frame['NominalBolometerProperties']

    def send_off(self, frame):
        if not self.wiring_map is None and self.bolo_props is None:

            #faking the frame data
            self.bolo_props = calibration.BolometerPropertiesMap()

            n_chans = 0
            squids = {}
            for k in self.wiring_map.keys():
                wm = self.wiring_map[k]
                c = wm.channel + 1

                if c > n_chans:
                    n_chans = c
                sq = get_physical_id(wm.board_serial, 
                                         wm.crate_serial, 
                                         wm.board_slot,
                                         wm.module + 1)
                squids[sq] = 1
            n_squids = len(squids.keys())

            sq_layout = make_square_block(n_squids)
            ch_layout = make_square_block(n_chans)

            sq_x_sep = ch_layout[0] + 1
            sq_y_sep = ch_layout[1] + 1
            ch_x_sep = 1
            ch_y_sep = 1

            for i, sq in enumerate( sorted(squids.keys()) ):
                x = i % sq_layout[0]
                y = i // sq_layout[0]
                squids[sq] = (1.2 * x * ch_layout[0], 1.2* y * ch_layout[1])

            #need nsquids
            #need nbolos per squid
            for k in self.wiring_map.keys():
                wm = self.wiring_map[k]
                sq_id = get_physical_id(wm.board_serial, 
                                            wm.crate_serial, 
                                            wm.board_slot,
                                            wm.module + 1)

                w_id = get_physical_id(wm.board_serial, 
                                           wm.crate_serial, 
                                           wm.board_slot)

                sql = squids[sq_id]
                x = sql[0] + ((wm.channel) % ch_layout[0]) * ch_x_sep
                y = sql[1] + ((wm.channel) // ch_layout[0]) * ch_y_sep

                bp = calibration.BolometerProperties()
                bp.physical_name = k
                bp.band = 0
                bp.pol_angle = 0
                bp.pol_efficiency = 0
                bp.wafer_id = w_id
                bp.squid_id = sq_id
                bp.x_offset = float(x) 
                bp.y_offset = float(y) 
                self.bolo_props[k] = bp

            out_frame = core.G3Frame(core.G3FrameType.Calibration)
            out_frame['BolometerProperties'] = self.bolo_props
            out_frame['DfMuxTransferFunction'] = self.default_tf
            return [out_frame, frame]
        else:
            return frame


        
class BirdConfigGenerator(object):
    def __init__(self, 
                 lyrebird_output_file = '',
                 get_hk_script_name= '',
                 hostname = '', hk_hostname = '',
                 port = 3, hk_port = 3, get_hk_port = 3,
                 dv_buffer_size = 0, min_max_update_interval = 0,
                 rendering_sub_sampling = 1, max_framerate = 0,
                 mean_decay_factor = 0.01
    ):
        self.l_fn = lyrebird_output_file
        self.get_hk_script_name = get_hk_script_name
        self.is_written = False
        self.bolo_props = None
        self.wiring_map = None
        self.hostname = hostname
        self.hk_hostname = hk_hostname
        self.port = port
        self.hk_port = hk_port
        self.get_hk_port = get_hk_port
        self.dv_buffer_size = dv_buffer_size
        self.min_max_update_interval = min_max_update_interval
        self.rendering_sub_sampling = rendering_sub_sampling
        self.max_framerate = max_framerate
        self.mean_decay_factor = mean_decay_factor
    def __call__(self, frame):
        if frame.type == core.G3FrameType.Calibration:
            if 'BolometerProperties' in frame:
                bp_id = 'BolometerProperties'
            elif 'NominalBolometerProperties' in frame:
                bp_id = 'NominalBolometerProperties'
            else:
                raise RuntimeError("BolometerProperties fucked")
            self.bolo_props = frame[bp_id]
            self.write_config()
        elif frame.type == core.G3FrameType.Wiring:
            self.wiring_map = frame['WiringMap']
            self.write_config()
    def write_config(self):
        if self.wiring_map is None or self.bolo_props is None:
            return
        config_dic = generate_dfmux_lyrebird_config(
            self.l_fn,
            self.wiring_map, self.bolo_props, 
            hostname = self.hostname,
            hk_hostname = self.hk_hostname,
            port = self.port,
            hk_port = self.hk_port,
            control_host = self.hostname,
            gcp_get_hk_port = self.get_hk_port,
            dv_buffer_size = self.dv_buffer_size,
            min_max_update_interval = self.min_max_update_interval,
            sub_sampling = self.rendering_sub_sampling,
            max_framerate = self.max_framerate,
            mean_decay_factor = self.mean_decay_factor
        )
        write_get_hk_script(self.get_hk_script_name, 
                            self.hostname, self.get_hk_port)
        print("Done writing config file")

class IdSerialMapper(object):
    def __init__(self, wiring_map):
        self.mp = {}
        self.mp_inv = {}
        for k in wiring_map.keys():
            wm = wiring_map[k]
            board_id = get_physical_id(wm.board_serial,
                                       wm.crate_serial,
                                       wm.board_slot)
            self.mp[ wm.board_serial ] = board_id
            self.mp_inv[board_id] = wm.board_serial
    def get_id(self, serial):
        return self.mp[serial]
    def get_serial(self, id):
        return self.mp_inv[id]

###########################
## Squid display portion ##
###########################
def add_timestamp_info(screen, y, x, ts, col_index):
    s = ts.Description()
    screen.addstr(y, x, s[:s.rfind('.')], curses.color_pair(col_index))

#need screen geometry and squid list and squid mapping
def add_squid_info(screen, y, x, 
                   sq_label, sq_label_size,
                   carrier_good, nuller_good, demod_good,
                   temperature_good, 
                   voltage_good,
                   max_size, 
                   bolometer_good, 
                   fir_stage,
                   #routing_good,
                   feedback_on,
                   bolo_label = '',
                   neutral_c = 3, good_c = 2, bad_c = 1):

    col_map = {True: curses.color_pair(good_c), 
               False: curses.color_pair(bad_c) }
    current_index = x
    screen.addstr(y, current_index, sq_label, curses.color_pair(neutral_c))
    current_index += sq_label_size

    screen.addstr(y, current_index, 'C', col_map[carrier_good])
    current_index += 1

    screen.addstr(y, current_index, 'N', col_map[nuller_good])
    current_index += 1

    screen.addstr(y, current_index, 'D', col_map[demod_good])
    current_index += 1

    screen.addstr(y, current_index, 'T', col_map[temperature_good])
    current_index += 1

    screen.addstr(y, current_index, 'V', col_map[voltage_good])
    current_index += 1

    screen.addstr(y, current_index, '%d'%fir_stage, col_map[fir_stage == 6])
    current_index += 1

    #screen.addstr(y, current_index, 'R', col_map[routing_good])
    #current_index += 1

    screen.addstr(y, current_index, 'F', col_map[feedback_on])
    current_index += 1

    if (not bolometer_good):
        screen.addstr(y, 
                      current_index, ' '+bolo_label[:(max_size - 7 - sq_label_size )], 
                      col_map[False])

def load_squid_info_from_hk( screen, y, x, 
                             hk_map,
                             sq_dev_id, sq_label, sq_label_size, 
                             max_size, serial_mapper):
    carrier_good = False
    nuller_good = False 
    demod_good = False
    temp_good = False 
    volt_good = False 
    bolometer_good = False
    full_label = 'NoData'
    fir_stage = 0
    routing_good = False

    feedback_on = False


    board_id, mezz_num, module_num = sq_phys_id_to_info(sq_dev_id)
    board_serial = serial_mapper.get_serial(board_id)

    #code for loading hk info for display
    if (not hk_map is None) and board_serial in hk_map:
        board_info = hk_map[board_serial]
        mezz_info = hk_map[board_serial].mezz[mezz_num]
        module_info = hk_map[board_serial].mezz[mezz_num].modules[module_num]

        
        fir_stage = int(board_info.fir_stage)



        routing_good = module_info.routing_type.lower() == 'routing_nul'
        feedback_on = module_info.squid_feedback.lower() == 'squid_lowpass'

        carrier_good = not module_info.carrier_railed
        nuller_good = not module_info.nuller_railed
        demod_good = not module_info.demod_railed

        def dic_range_check(dr, dv):
            for k in dv.keys():
                if (not k in dr):
                    continue
                rng = dr[k]
                v = dv[k]
                if v < rng[0] or v > rng[1]:
                    return False
            return True

        voltage_range = {'MOTHERBOARD_RAIL_VCC5V5': (5,6),
                         'MOTHERBOARD_RAIL_VADJ': (2,3),
                         'MOTHERBOARD_RAIL_VCC3V3': (3,3.6),
                         'MOTHERBOARD_RAIL_VCC1V0': (0.8, 1.2),
                         'MOTHERBOARD_RAIL_VCC1V2': (1, 1.5), 
                         'MOTHERBOARD_RAIL_VCC12V0': (11, 13), 
                         'MOTHERBOARD_RAIL_VCC1V8': (1.6, 2), 
                         'MOTHERBOARD_RAIL_VCC1V5': (1.3, 1.7), 
                         'MOTHERBOARD_RAIL_VCC1V0_GTX': (0.7, 1.3)}

        temp_range = {'MOTHERBOARD_TEMPERATURE_FPGA': (0,80), 
                      'MOTHERBOARD_TEMPERATURE_POWER': (0,80),
                      'MOTHERBOARD_TEMPERATURE_ARM': (0,80),
                      'MOTHERBOARD_TEMPERATURE_PHY': (0,80)}
        
        #mezz voltages
        mezz_voltage_range = {'MEZZANINE_RAIL_VCC12V0': (11,13), 
                               'MEZZANINE_RAIL_VADJ': (2,3), 
                               'MEZZANINE_RAIL_VCC3V3': (3,4) }

        temp_good = dic_range_check( temp_range, board_info.temperatures)

        volt_good = ( dic_range_check( voltage_range, board_info.voltages) or
                      dic_range_check( mezz_voltage_range, mezz_info.voltages)
                  )

        bolometer_good = True
        bolo_label = ''


        n_railed = 0
        n_diff_freq = 0
        n_dan_off = 0
        for b in module_info.channels.keys():
            chinfo = module_info.channels[b]
            if (chinfo.dan_railed):
                n_railed += 1
            elif (chinfo.carrier_frequency != chinfo.demod_frequency):
                n_diff_freq += 1
            elif ( (not (chinfo.dan_accumulator_enable and
                         chinfo.dan_feedback_enable and
                         chinfo.dan_streaming_enable ) )
                   and (chinfo.carrier_frequency > 0  and chinfo.carrier_amplitude > 0) ):
                n_dan_off += 1
                      
                
        bolometer_good = not (n_railed or n_diff_freq or n_dan_off)
        
        if not bolometer_good:
            if n_railed:
                full_label = "DanRail:%s"%(n_railed)
            elif n_diff_freq:
                full_label = "CDDiffFreq:%s"%(n_diff_freq)
            elif n_dan_off:
                full_label = "DanOff:%s"%(n_dan_off)
        else:
            full_label = ''



    add_squid_info(screen, y, x, 
                   sq_label, sq_label_size,
                   carrier_good, nuller_good, demod_good,
                   temp_good, volt_good,
                   max_size,
                   bolometer_good, 
                   fir_stage,
                   #routing_good,
                   feedback_on,
                   bolo_label = full_label,
    )

def GetHousekeepingMessenger(frame, hostname, port):
    if frame.type == core.G3FrameType.Wiring:
        os.system( "nc %s %d" % (hostname, port) )

class SquidDisplay(object):
    def __init__(self,  
                 squids_per_col = 32, 
                 squid_col_width = 30):
        self.squids_list = None
        self.squids_per_col = squids_per_col
        self.squid_col_width = squid_col_width
        self.serial_mapper = None
        self.str_id_lst = ["       Carrier",
                           "       Nuller",
                           "       Demod",
                           "       Temp",
                           "       Voltage",
                           "    fir#",
                           " squid Feedback"
        ]
        self.highlight_index = [7 for s in self.str_id_lst]

    def init_squids(self, squids_list) :
        self.n_squids = len(squids_list) + len(self.str_id_lst) + 1
        self.squids_list = squids_list

        self.sq_label_size = max(map(len, squids_list)) + 3        
        ncols = int(math.ceil(float(self.n_squids)/self.squids_per_col))

        self.screen_size_x = ncols * self.squid_col_width
        self.screen_size_y = self.squids_per_col + 2

        self.pos_map = {}
        #assign an x, y location to each squid
    
        for j, sq in enumerate(sorted(squids_list, key=cmp_to_key(str_cmp_with_numbers_sorted))):
            i = j + len(self.str_id_lst) + 1
            y =  i % self.squids_per_col + 1
            x = 1 + self.squid_col_width * ( i // self.squids_per_col)
            self.pos_map[sq] = (x,y)

        self.stdscr = curses.initscr()
        
        curses.start_color()
            
        # Turn off echoing of keys, and enter cbreak mode,
        # where no buffering is performed on keyboard input
        curses.noecho()
        curses.cbreak()
        curses.curs_set(0)
        
        curses.init_pair(1, curses.COLOR_RED,     curses.COLOR_WHITE)
        curses.init_pair(2, curses.COLOR_GREEN,   curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_BLUE,    curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_YELLOW,  curses.COLOR_BLACK)
        curses.init_pair(5, curses.COLOR_BLUE,    curses.COLOR_WHITE)
        
        self.stdscr.clear()
        signal.signal(signal.SIGWINCH, signal.SIG_IGN)

    def __call__(self, frame):
        if frame.type == core.G3FrameType.Wiring:
            wiring_map = frame['WiringMap']
            squid_ids = []
            for k in wiring_map.keys():
                wm = wiring_map[k]
                squid_ids.append( get_physical_id(wm.board_serial, 
                                                  wm.crate_serial, 
                                                  wm.board_slot,
                                                  wm.module + 1) )
            squid_ids = uniquifyList(squid_ids)
            self.init_squids(squid_ids)
            self.serial_mapper = IdSerialMapper(frame['WiringMap'])

        elif frame.type == core.G3FrameType.Housekeeping:
            if self.squids_list is None:
                return
            #do update
            if not frame is None:
                hk_data = frame['DfMuxHousekeeping']
            else:
                hk_data = None
            self.stdscr.clear()

            y, x = self.stdscr.getmaxyx()
            if y < self.screen_size_y or x < self.screen_size_x:
                screen = self.stdscr.subwin(0, x, 0, 0)
                screen.addstr(0,0, 'Terminal is too small %d %d'%(y,x), curses.color_pair(1))
                screen.refresh()
                return

            screen = self.stdscr.subwin(0, self.screen_size_x, 0, 0)
            screen.clear()


            #screen.box()
            #CNDTV6F
            if not hk_data is None:
                add_timestamp_info(screen, 0,2, hk_data[hk_data.keys()[0]].timestamp, 5)
                for i, s in enumerate(self.str_id_lst):
                    offset = 4
                    screen.addstr(i+1, offset, s, curses.color_pair(2))
                    screen.addstr(i+1, offset + self.highlight_index[i], 
                                       s[self.highlight_index[i]], curses.color_pair(3))
                    
                screen.hline(len(self.str_id_lst) + 1, 0, 
                                  '-', self.squid_col_width)
                screen.vline(0, self.squid_col_width-1, 
                                  '|', len(self.str_id_lst)+1)

            for i, s in enumerate(self.squids_list):
                p = self.pos_map[s]
                load_squid_info_from_hk( screen, p[1], p[0], 
                                         hk_data,
                                         s, s, self.sq_label_size, 
                                         self.squid_col_width, self.serial_mapper)
            screen.refresh()
        elif frame.type == core.G3FrameType.EndProcessing:
            if not self.squids_list is None:
                self.stdscr.keypad(0)
                curses.echo()
                curses.nocbreak()
                curses.endwin() 


if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('hostname')
    parser.add_argument('--port',type=int, default=8675)
    parser.add_argument('--local_ts_port',type=int, default=8676)
    parser.add_argument('--local_hk_port',type=int, default=8677)
    parser.add_argument('--gcp_signalled_hk_port', type=int, default=50011)
    parser.add_argument('--lyrebird_output_file', default = 'lyrebird_config_file.json')
    parser.add_argument('--get_hk_script', default = 'get_hk.sh')

    parser.add_argument('--timestream_buffer_size',type=int, default=1024)
    parser.add_argument('--min_max_update_interval', type=int, default = 300)

    parser.add_argument('--rendering_sub_sampling', type=int, default = 2)
    parser.add_argument('--max_framerate', type=int, default = 60)

    parser.add_argument("--mean_decay_factor", type = float, default = 0.01, 
                        help = "The mean filtered power has an exponential convolution form to the filter.  It has a value in (0,1) exclusive.  Increasing the value decreases the size of the exponential to it pushes the frequency of the HPF lower.  Numbers close to one filter things very rapidly, close to 0 very slowly.")
    parser.add_argument('--debug_mode', action='store_true', help = "prevents the spawning on the curses display")
    parser.add_argument('--debug_logs', action='store_true', help = "store logs of stderr/out")
    parser.add_argument('--ignore_nominal_bias_props', action='store_true', help = "will align the bolometers into a grid")
    


    args = parser.parse_args()
    #core.set_log_level(core.G3LogLevel.LOG_DEBUG)

    script_path = os.path.dirname(os.path.realpath(__file__))
    script_path = script_path + '/../bin/'

    lyrebird_output_file = script_path + args.lyrebird_output_file
    get_hk_script = script_path + args.get_hk_script

    pipe = core.G3Pipeline()
    pipe.Add(core.G3NetworkReceiver, 
             hostname = args.hostname, port = args.port)

    if args.ignore_nominal_bias_props:
        pipe.Add(lambda fr: fr.type != core.G3FrameType.Calibration)

    pipe.Add(BoloPropertiesFaker)

    pipe.Add(AddVbiasAndCurrentConv)

    pipe.Add(BirdConfigGenerator, 
             lyrebird_output_file = lyrebird_output_file, 
             hostname = args.hostname, 
             get_hk_script_name = get_hk_script,
             hk_hostname = '127.0.0.1',
             port = args.local_ts_port, 
             hk_port = args.local_hk_port,
             get_hk_port = args.gcp_signalled_hk_port,
             dv_buffer_size = args.timestream_buffer_size,
             min_max_update_interval = args.min_max_update_interval,
             rendering_sub_sampling = args.rendering_sub_sampling,
             max_framerate = args.max_framerate,
             mean_decay_factor = args.mean_decay_factor
    )

    pipe.Add(GetHousekeepingMessenger, hostname = args.hostname, 
             port = args.gcp_signalled_hk_port)

    pipe.Add(core.G3ThrottledNetworkSender,
             hostname = '*',
             port = args.local_hk_port,
             frame_decimation = {core.G3FrameType.Timepoint: 10}
          )

    pipe.Add(core.G3ThrottledNetworkSender,
             hostname = '*',
             port = args.local_ts_port,
             frame_decimation = {core.G3FrameType.Housekeeping: 0}
          )

    if args.debug_logs:
        import sys
        sys.stderr = open('kookaburra_stderr.txt', 'w')
        sys.stdout = open('kookaburra_stdout.txt', 'w')

    if args.debug_mode:
        pipe.Add(core.Dump)

        pipe.Run()
    else:
        pipe.Add(SquidDisplay)

        try:
            pipe.Run()
        finally:
            traceback.print_exc()  # Print the exception
            curses.curs_set(1)
            curses.echo()
            curses.nocbreak()
            curses.endwin()

