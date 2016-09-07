import argparse, json, os
import config_constructor as CC
import numpy as np
from operator import itemgetter, attrgetter

N_MODULES=8
N_CHANNELS=64

def get_physical_id(board_serial, crate_serial, board_slot,
                    module = None, channel = None):
    if board_slot == 0:
        s = '%d_%d' % (crate_serial, board_slot)
        
    if module != None:
        s += '/%d'%module
        if channel != None:
            s += '/%d' % channel
    return s

def sq_phys_id_to_info(phys_id):
    split = phys_id.split('/')
    board_id = split[0]
    module_num = int(split[1])
    mezz_num = module_num // 4
    module_num = module_num % 4
    return board_id, mezz_num, module_num

def uniquifyList(seq):
    checked = []
    for e in seq:
        if e not in checked:
            checked.append(e)
    return checked


def generate_dfmux_eq_lazy(devid, vlabel, label = None):
    if label == None:
        vlabel = label
    return CC.getEquation('%s:%s'%(devid, vlabel), 
                          'white_cmap', 
                          '%s:%s'%(devid, vlabel)+'_eq',
                          label,
                          '%s:%s'%(devid, vlabel)
                      )
def get_board_vals():
    return ['fir_stage']

def get_module_vals():
    return ['carrier_railed', 'nuller_railed', 'demod_railed', 'squid_flux_bias',
            'squid_current_bias', 'squid_stage1_offset', 'squid_feedback',
            'routing_type']

def get_channel_vals():
    return ['carrier_amplitude', 'carrier_frequency', 'demod_frequency',
            'dan_accumulator_enable', 'dan_feedback_enable', 'dan_streaming_enable', 
            'dan_gain', 'dan_railed']


def addDfmuxStreamer(config_dic, tag, boards_list, 
                    sender_hostname = 'laphroaig.berkeley.edu',
                    sender_port = 8675):
    desc = {'board_list':boards_list,
            'network_streamer_hostname': sender_hostname, 
            'network_streamer_port': sender_port }
    glob_eqs = []
    for b in boards_list:
        for bhv in get_board_vals():
            CC.addGlobalEquation(config_dic, generate_dfmux_eq_lazy(b, bhv))
        for m in range(N_MODULES):
            mid = '%s/%d' %(b,m)
            for mhv in get_module_vals():
                CC.addGlobalEquation(config_dic, generate_dfmux_eq_lazy(mid, mhv))
            for c in range(N_CHANNELS):
                cid = '%s/%d/%d' %(b,m,c)
                CC.addGlobalEquation(config_dic, 
                                     generate_dfmux_eq_lazy(cid + '/I', 
                                                            "dfmux_samples", 'I_Samples'))
                CC.addGlobalEquation(config_dic, 
                                     generate_dfmux_eq_lazy(cid + '/Q', 
                                                            "dfmux_samples", 'Q_Samples'))
                for chv in get_channel_vals():
                    CC.addGlobalEquation(config_dic, generate_dfmux_eq_lazy(cid,chv))
    CC.addDataSource(config_dic, tag, 'dfmux',  desc)
    return glob_eqs

def addDfmuxVisElems(config_dic, wiring_map, bolo_props_map, 
                     scale_fac, svg_folder):
    for k in bolo_props_map.keys():
        if (not k in wiring_map):
            continue
        bp = bolo_props_map[k]
        wm = wiring_map[k]

        bid = get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot)
        mid = get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot,
                              wm.module )
        cid = get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot,
                              wm.module, wm.channel )

        if bp.band == 150:
            svg = svg_folder + 'medpol.svg'
            h_svg = svg_folder + 'medhighligh.svg'
            group = '150s'
        elif bp.band == 90:
            svg = svg_folder + 'largepol.svg'
            h_svg = svg_folder + 'largehighligh.svg'
            group = '90s'

        elif bp.band == 220:
            svg = svg_folder + 'smallpol.svg'
            h_svg = svg_folder + 'smallhighligh.svg'
            group = '220s'

        else:
            svg = svg_folder + 'box.svg'
            h_svg = svg_folder + 'boxhighligh.svg'
            group = 'Misfit Toys'

        eqs_lst = ['%s/I:dfmux_samples_eq' % cid, 
                   '%s/Q:dfmux_samples_eq' % cid ]
        for mvs in get_module_vals():
            eqs_lst.append('%s:%s_eq' % (mid, mvs))
        for cvs in get_channel_vals():
            eqs_lst.append('%s:%s_eq' % (cid, cvs))
        for bvs in get_board_vals():
            eqs_lst.append('%s:%s_eq' % (bid, bvs))
            
        CC.addVisElem(config_dic, 
                      x_cen=bp.x_offset,   y_cen=bp.y_offset,
                      x_scale = scale_fac, y_scale=scale_fac, 
                      rotation= bp.pol_angle,
                      svg_path= svg,
                      highlight_path = h_svg,
                      layer = 1, labels = [cid],
                      equations = eqs_lst, 
                      labelled_data = ["Board", bid, 
                                       "Module", cid,
                                       "PhysId", bp.physical_name
                                   ],
                      group = group )

        

def generate_dfmux_lyrebird_config(wiring_map, bolo_props_map, 
                                   win_x_size = 800,
                                   win_y_size = 800,
                                   sub_sampling = 4,
                                   max_framerate = -1,
                                   max_num_plotted  = 70,
                                   hostname = '127.0.0.1',
                                   port = 8675):
    svg_folder = '../svgs/'


    cell_size = 100
    safety_factor = 1.2
    min_delt = 1e-5
    board_ids = []
    #need to get boards list
    for k in wiring_map.keys():
        wm = wiring_map[k]
        board_ids.append( get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot) )
    board_ids = uniquifyList(board_ids)
    
    squid_ids = []
    #need to get boards list
    for k in wiring_map.keys():
        wm = wiring_map[k]
        squid_ids.append( get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot,
                                          wm.module ) )
    squid_ids = uniquifyList(squid_ids)
    
    channel_ids = []
    #need to get boards list
    for k in wiring_map.keys():
        wm = wiring_map[k]
        channel_ids.append( get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot,
                                            wm.module, wm.channel
                                        ) )
    channel_ids = uniquifyList(channel_ids)
    
    #need to get smallest spacing
    special_key = bolo_props_map.keys()[0]
    bp_0 = bolo_props_map[special_key]

    max_delt = 1e12
    for k in bolo_props_map.keys():
        bp =  bolo_props_map[k]
        delt = (bp_0.x_offset- bp.x_offset)**2.0 + (bp_0.y_offset- bp.y_offset)**2.0 
        if delt < max_delt and delt > min_delt:
            max_delt = delt
    special_separation = max_delt**0.5
    scale_fac = safety_factor * special_separation / cell_size

    #add the general settings
    config_dic = {}
    addGeneralSettings(config_dic, 
                       win_x_size = win_x_size,
                       win_y_size = win_y_size,
                       sub_sampling = sub_sampling,
                       max_framerate= max_framerate,
                       max_num_plotted = max_num_plotted)
    
    addDfmuxStreamer(config_dic, "dfmux_streamer", board_ids, 
                     sender_hostname = hostname,
                     sender_port = port)

    addDfmuxVisElems(config_dic, wiring_map, bolo_props_map, 
                     scale_fac, svg_folder)
    return config_dic, board_ids, module_ids, channel_ids

    
                           
