from spt3g import core, dfmux, networkstreamer, auxdata
import argparse, json, os, math
from operator import itemgetter, attrgetter
import config_constructor as CC
import dfmux_config_constructor as DCC
import kookaburra as birdyoucantspell


parser = argparse.ArgumentParser()
parser.add_argument('hostname')
parser.add_argument('port',type=int)
parser.add_argument('local_port',type=int)
parser.add_argument('lyrebird_output_file')
parser.add_argument('kookaburra_output_file')
args = parser.parse_args()


def make_square_block(n_things):
    sq = n_things**0.5
    if n_things == int(math.floor(sq))**2:
        return (sq,sq)
    else:
        sq = int(math.floor(sq))
        return (sq, sq+1)


class BoloPropertiesFaker(object):
    def __init__(self):
        self.wiring_map = None
        self.bolo_props = None
        self.sent_off = False
        return

    def __call__(self, frame):
        if frame.type == core.G3FrameType.Wiring:
            self.wiring_map = frame['WiringMap']
        elif frame.type == core.G3FrameType.Calibration:
            if 'BolometerProperties' in frame:
                self.bolo_props = frame['BolometerProperties']
            elif 'NominalBolometerProperties' in frame:
                self.bolo_props = frame['NominalBolometerProperties']
        elif frame.type == core.G3FrameType.Timepoint:
            if not self.sent_off:
                self.sent_off = True
                if self.bolo_props != None:
                    return
                #faking the frame data
                self.bolo_props = auxdata.BolometerPropertiesMap()

                n_chans = 0
                squids = {}
                for k in self.wiring_map.keys():
                    wm = self.wiring_map[k]
                    c = wm.channel

                    if c > n_chans:
                        n_chans = c
                    sq = DCC.get_physical_id(wm.board_serial, 
                                             wm.crate_serial, 
                                             wm.board_slot,
                                             wm.module)
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
                    squids[sq] = (x * ch_layout[0], y * ch_layout[1])

                #need nsquids
                #need nbolos per squid
                for k in self.wiring_map.keys():
                    wm = self.wiring_map[k]
                    sq_id = DCC.get_physical_id(wm.board_serial, 
                                                wm.crate_serial, 
                                                wm.board_slot,
                                                wm.module)
                    
                    w_id = DCC.get_physical_id(wm.board_serial, 
                                               wm.crate_serial, 
                                               wm.board_slot)

                    sql = squids[sq_id]
                    x = sql[0] + ((wm.channel - 1) % ch_layout[0]) * ch_x_sep
                    y = sql[1] + ((wm.channel - 1) // ch_layout[0]) * ch_y_sep

                    bp = auxdata.BolometerProperties()
                    bp.physical_name = k
                    bp.band = 0
                    bp.pol_angle = 0
                    bp.pol_efficiency = 0
                    bp.wafer_id = w_id
                    bp.squid_id = sq_id
                    bp.x_offset = x
                    bp.y_offset = y
                    self.bolo_props[k] = bp

                out_frame = core.G3Frame(core.G3FrameType.Calibration)
                out_frame['BolometerProperties'] = self.bolo_props
                return [out_frame, frame]
        return
        
class BirdConfigGenerator(object):
    def __init__(self, 
                 lyrebird_output_file = '',
                 kookaburra_output_file = '',
                 hostname = '',
                 port = 3
    ):
        self.l_fn = lyrebird_output_file
        self.k_fn = kookaburra_output_file
        self.is_written = False
        self.bolo_props = None
        self.wiring_map = None
        self.hostname = hostname
        self.port = port

    def __call__(self, frame):
        if frame.type == core.G3FrameType.Calibration:
            if 'BolometerProperties' in frame:
                bp_id = 'BolometerProperties'
            elif 'BolometerPropertiesNominal' in frame:
                bp_id = 'BolometerPropertiesNominal'
            else:
                raise RuntimeError("bp fucked")
            self.bolo_props = frame[bp_id]
        elif frame.type == core.G3FrameType.Wiring:
            self.wiring_map = frame['WiringMap']
        else:
            if self.is_written or self.wiring_map == None or self.bolo_props == None:
                return
            self.is_written = True
            config_dic, board_ids, module_ids, channel_ids = DCC.generate_dfmux_lyrebird_config(
                self.wiring_map, self.bolo_props, 
                hostname = self.hostname,
                port = self.port)
            CC.storeConfigFile(config_dic, self.l_fn) 
            birdyoucantspell.generate_kookie_config_file(self.k_fn, 
                                                         module_ids, module_ids,
                                                         channel_ids,
                                                         self.hostname, self.port)

pipe = core.G3Pipeline()
pipe.Add(networkstreamer.G3NetworkReceiver, hostname = args.hostname, port = args.port)
pipe.Add(networkstreamer.G3NetworkSender, ip = '127.0.0.1', port = args.local_port)
pipe.Add(BoloPropertiesFaker)
pipe.Add(BirdConfigGenerator, 
         lyrebird_output_file = args.lyrebird_output_file, 
         kookaburra_output_file = args.kookaburra_output_file, 
         hostname = '127.0.0.1', port = args.local_port)
#pipe.Add(core.Dump)

pipe.Run()
