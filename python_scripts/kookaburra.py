import numpy as np
import socket, curses, json, traceback, math
from dfmux_config_constructor import get_physical_id, sq_phys_id_to_info
from spt3g import core, dfmux, networkstreamer
from spt3g.core import genericutils as GU

import warnings
warnings.filterwarnings("ignore")

def read_kookie_config_file(fn):
    d = json.load(open(fn))
    return d

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
    screen.addstr(y, x, ts.Description(), curses.color_pair(col_index))

#need screen geometry and squid list and squid mapping
def add_squid_info(screen, y, x, 
                   sq_label, sq_label_size,
                   carrier_good, nuller_good, demod_good,
                   temperature_good, 
                   voltage_good,
                   max_size,
                   bolometer_good, bolo_label = '',
                   neutral_c = 3, good_c = 2, bad_c = 1):
    assert( (7 + sq_label_size) < max_size)
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

    screen.addstr(y, current_index, 'B', col_map[bolometer_good])
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


    board_id, mezz_num, module_num = sq_phys_id_to_info(sq_dev_id)
    board_serial = serial_mapper.get_serial(board_id)


    #code for loading hk info for display
    if hk_map != None and board_serial in hk_map:
        board_info = hk_map[board_serial]
        mezz_info = hk_map[board_serial].mezz[mezz_num]
        module_info = hk_map[board_serial].mezz[mezz_num].modules[module_num]

        carrier_good = not module_info.carrier_railed
        nuller_good = not module_info.nuller_railed
        demod_good = not module_info.demod_railed

        def dic_range_check(dr, dv):
            for k in dv.keys():
                assert(k in dr)
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
        n_bad_bolo = 0

        #check for carrier demod same frequency
        #check if dan is on
        #squid routing
        #squid feedback
        #fir 
        #timestamp port
        #serial

        #valid routing types of the module

        for b in module_info.channels.keys():
            chinfo = module_info.channels[b]
            if (chinfo.dan_railed):
                n_bad_bolo += 1
                bolometer_good = False
                bolo_label = 'Railed'
        full_label = "%d:%s"%(n_bad_bolo, bolo_label)

    add_squid_info(screen, y, x, 
                   sq_label, sq_label_size,
                   carrier_good, nuller_good, demod_good,
                   temp_good, volt_good,
                   max_size,
                   bolometer_good, full_label)

class SquidDisplay(object):
    def __init__(self, squids_list, 
                 squids_per_col = 32, 
                 squid_col_width = 30):
        self.squids_list = squids_list
        self.squids_per_col = squids_per_col
        self.squid_col_width = squid_col_width
        self.n_squids = len(squids_list)

        self.sq_label_size = max(map(len, squids_list)) + 3        
        ncols = int(math.ceil(float(self.n_squids)/self.squids_per_col))

        self.screen_size_x = ncols * squid_col_width
        self.screen_size_y = self.squids_per_col + 2

        self.pos_map = {}
        #assign an x, y location to each squid

        self.serial_mapper = None
        for i, sq in enumerate(sorted(squids_list, cmp = GU.str_cmp_with_numbers_sorted)):
            y =  i % self.squids_per_col + 1
            x = 1 + self.squid_col_width * ( i // self.squids_per_col)
            self.pos_map[sq] = (x,y)

        self.stdscr = curses.initscr()

        y, x = self.stdscr.getmaxyx()
        if y < self.screen_size_y:
            raise RuntimeError("screen is not tall enough, extend to %d", self.screen_size_y)
        if x < self.screen_size_x:
            raise RuntimeError("screen is not wide enough, extend to %d", self.screen_size_x)

        curses.start_color()
            
        # Turn off echoing of keys, and enter cbreak mode,
        # where no buffering is performed on keyboard input
        curses.noecho()
        curses.cbreak()
        curses.curs_set(0)
        
        self.screen = self.stdscr.subwin(0, self.screen_size_x, 0, 0)

        curses.init_pair(1, curses.COLOR_RED,     curses.COLOR_WHITE)
        curses.init_pair(2, curses.COLOR_GREEN,   curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_BLUE,    curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_YELLOW,  curses.COLOR_BLACK)


        self.stdscr.clear()
        self.screen.clear()
        self.screen.refresh()

    def __call__(self, frame):
        if frame == None or frame.type == core.G3FrameType.Housekeeping:
            #do update
            if frame != None:
                hk_data = frame['DfMuxHousekeeping']
            else:
                hk_data = None
            self.stdscr.clear()
            self.screen.clear()
            self.screen.box()

            if hk_data != None:
                add_timestamp_info(self.screen, 0,0, hk_data[hk_data.keys()[0]].timestamp,
                                   3)

            for i, s in enumerate(self.squids_list):
                p = self.pos_map[s]
                load_squid_info_from_hk( self.screen, p[1], p[0], 
                                         hk_data,
                                         s, s, self.sq_label_size, 
                                         self.squid_col_width, self.serial_mapper)
            self.screen.refresh()
        elif frame.type == core.G3FrameType.EndProcessing:
            self.stdscr.keypad(0)
            curses.echo()
            curses.nocbreak()
            curses.endwin() 
        elif frame.type == core.G3FrameType.Wiring:
            self.serial_mapper = IdSerialMapper(frame['WiringMap'])

if __name__=='__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('config_file')
    args = parser.parse_args()

    conf_dic = read_kookie_config_file(args.config_file)

    print conf_dic
    try:
        pipe = core.G3Pipeline()
        pipe.Add(networkstreamer.G3NetworkReceiver, 
                 hostname = str(conf_dic['listen_hostname']), 
                 port = int(conf_dic['listen_port']))
        pipe.Add(core.Dump)
        pipe.Add(SquidDisplay, squids_list = conf_dic['squid_dev_id_list'])
        pipe.Run()
    finally:
        curses.curs_set(1)
        curses.echo()
        curses.nocbreak()
        curses.endwin()
        traceback.print_exc()  # Print the exception

