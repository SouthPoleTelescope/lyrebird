import argparse, json, os
import config_constructor as CC
import numpy as np
from operator import itemgetter, attrgetter
from spt3g import core

N_MODULES=8
N_CHANNELS=64

fl = lambda x: float(np.nan_to_num(x))

def get_physical_id(board_serial, crate_serial, board_slot,
                    module = None, channel = None):
    if board_slot <= 0:
        s = '%d' % board_serial
    else:
        s = '%d_%d' % (crate_serial, board_slot)

    if module != None:
        s += '/%d'%module
        if channel != None:
            s += '/%d' % channel
    return s

def sq_phys_id_to_info(phys_id):
    split = phys_id.split('/')
    board_id = split[0]
    module_num = int(split[1]) - 1
    mezz_num = module_num // 4 + 1
    module_num = module_num % 4 + 1
    return board_id, mezz_num, module_num

def uniquifyList(seq):
    checked = []
    for e in seq:
        if e not in checked:
            checked.append(e)
    return checked


def generate_dfmux_eq_lazy(devid, vlabel, label = None):
    if label == None:
        label = vlabel
    return CC.getEquation('%s:%s'%(devid, vlabel), 
                          'white_cmap', 
                          '%s:%s'%(devid, vlabel)+'_eq',
                          label,
                          '%s:%s'%(devid, vlabel)
                      )
def get_board_vals():
    return ['fir_stage']

def get_module_vals():
    return ['carrier_gain', 'nuller_gain',
            'carrier_railed', 'nuller_railed', 'demod_railed', 'squid_flux_bias',
            'squid_current_bias', 'squid_stage1_offset', 'squid_feedback',
            'routing_type']

def get_channel_vals():
    return ['carrier_amplitude', 'carrier_frequency', 'demod_frequency',
            'dan_accumulator_enable', 'dan_feedback_enable', 'dan_streaming_enable', 
            'dan_gain', 'dan_railed', 'rnormal', 'rlatched']


def addDfmuxStreamer(config_dic, tag, boards_list, 
                     bolo_list,
                     mean_decay_factor = 0.05,
                     sender_hostname = '',
                     sender_port = 8675,
                     hk_hostname = '',
                     hk_port = 8676):
    hk_desc = {'board_list':boards_list,
               'network_streamer_hostname': hk_hostname, 
               'network_streamer_port': hk_port,
               'streamer_type': 1,
               'bolo_list': bolo_list
              }

    tp_desc = {'board_list':boards_list,
               'network_streamer_hostname': sender_hostname, 
               'network_streamer_port': sender_port,
               'streamer_type': 2,
               'mean_decay_factor': mean_decay_factor
              }

    glob_eqs = []
    for b in boards_list:
        for bhv in get_board_vals():
            CC.addGlobalEquation(config_dic, generate_dfmux_eq_lazy(b, bhv))
        for m in range(1, N_MODULES + 1):
            mid = '%s/%d' %(b,m)
            for mhv in get_module_vals():
                CC.addGlobalEquation(config_dic, generate_dfmux_eq_lazy(mid, mhv))

                                            
            CC.addGlobalEquation(config_dic, 
                                 CC.getEquation('| %s:carrier_railed | %s:nuller_railed %s:demod_railed'%(mid, mid, mid), 
                                                "rainbow_cmap",
                                                '%s:SquidGood'%(mid)+'_eq',
                                                "SQUID Be F*cked",
                                                '%s:carrier_railed'%(mid)))


            for c in range(1, N_CHANNELS + 1):
                cid = '%s/%d/%d' %(b,m,c)
                CC.addGlobalEquation(config_dic, 
                                     generate_dfmux_eq_lazy(cid + '/I', 
                                                            "dfmux_samples", 'I_Samples'))
                CC.addGlobalEquation(config_dic, 
                                     generate_dfmux_eq_lazy(cid + '/Q', 
                                                            "dfmux_samples", 'Q_Samples'))
                for chv in get_channel_vals():
                    CC.addGlobalEquation(config_dic, generate_dfmux_eq_lazy(cid,chv))
    CC.addDataSource(config_dic, 'tp_' + tag, 'dfmux',  tp_desc)
    CC.addDataSource(config_dic, 'hk_' + tag, 'dfmux',  hk_desc)
    return glob_eqs

def addDfmuxVisElems(config_dic, wiring_map, bolo_props_map, 
                     scale_fac, svg_folder, max_freq = 6e6):
    for k in bolo_props_map.keys():
        if (not k in wiring_map):
            continue
        bp = bolo_props_map[k]
        wm = wiring_map[k]

        bid = get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot)
        mid = get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot,
                              wm.module + 1)
        cid = get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot,
                              wm.module + 1, wm.channel + 1 )
        
        if bp.band == 150*core.G3Units.GHz:
            svg = svg_folder + 'medpol.svg'
            h_svg = svg_folder + 'medhighlight.svg'
            group = '150s'
            eq_cmap = 'bolo_cyan_cmap'
        elif bp.band == 90*core.G3Units.GHz:
            svg = svg_folder + 'largepol.svg'
            h_svg = svg_folder + 'largehighlight.svg'
            group = '90s'
            eq_cmap = 'bolo_green_cmap'

        elif bp.band == 220*core.G3Units.GHz:
            svg = svg_folder + 'smallpol.svg'
            h_svg = svg_folder + 'smallhighlight.svg'
            group = '220s'
            eq_cmap = 'bolo_blue_cmap'

        else:
            svg = svg_folder + 'box.svg'
            h_svg = svg_folder + 'boxhighlight.svg'
            group = 'Misfit Toys'
            eq_cmap = 'bolo_cyan_cmap'
        '''
        CC.addGlobalEquation(config_dic, 
                             CC.getEquation('/ / %s:voltage_bias %s:current_conv * %s/I:dfmux_samples %s:rnormal'%(k, k, cid, cid),
                                            eq_cmap,
                                            '%s:Rfractional'%(cid)+'_eq',
                                            "Rfrac",
                                            '%s/I:dfmux_samples'%(cid)))
        '''
        CC.addGlobalEquation(config_dic, 
                             CC.getEquation((
                                 '* ! = %s:carrier_amplitude 0 ' +
                                 '* ! = %s:carrier_frequency 0 ' +
                                 '/ / %s:voltage_bias %s:current_conv * %s:rnormal q + * %s/I:dfmux_samples %s/I:dfmux_samples * %s/Q:dfmux_samples %s/Q:dfmux_samples') %
                                             (cid, cid, k, k, cid, cid, cid, cid, cid)),
                                            eq_cmap,
                                            '%s:Rfractional'%(cid)+'_eq',
                                            "Rfrac",
                                            '%s/I:dfmux_samples'%(cid))


        CC.addGlobalEquation(config_dic, 
                             CC.getEquation(('/ / %s:voltage_bias %s:current_conv q + * %s/I:dfmux_samples %s/I:dfmux_samples * %s/Q:dfmux_samples %s/Q:dfmux_samples' %
                                             (k, k, cid, cid, cid, cid)),
                                            eq_cmap,
                                            '%s:Resistance'%(cid)+'_eq',
                                            "Res",
                                            '%s/I:dfmux_samples'%(cid)))
        CC.addGlobalEquation(config_dic, 
                             CC.getEquation('* ! = %s:carrier_amplitude 0 T %s/Q:dfmux_samples %s/I:dfmux_samples'%(cid, cid, cid), 
                                            "phase_cmap",
                                            '%s:phase'%(cid)+'_eq',
                                            "Channel Phase",
                                            '%s/I:dfmux_samples'%(cid),
                                            display_in_info_bar = True,
                                        ))

        #k is the bolo id for vbias and iconv

        # '%s/I:dfmux_samples_mean_filtered' cid
        # '%s/Q:dfmux_samples_mean_filtered' cid
        # '%s:voltage_bias' k
        # '%s:current_conv' k

        CC.addGlobalEquation(config_dic, 
                             CC.getEquation(
                                 '* ! = %s:carrier_amplitude 0 / * * * %s:voltage_bias %s:current_conv %s/I:dfmux_samples_mean_filtered 1e15 PowScaling(fW)'%(cid, k, k, cid), 
                                 "rainbow_cmap_fs",
                                 '%s:IPower'%(cid)+'_eq',
                                 "I Power Scaled",
                                 '%s/I:dfmux_samples'%(cid),
                                 display_in_info_bar = True
                             ))


        CC.addGlobalEquation(config_dic, 
                             CC.getEquation(
                                 '* ! = %s:carrier_amplitude 0 / * * * %s:voltage_bias %s:current_conv %s/Q:dfmux_samples_mean_filtered 1e15 PowScaling(fW)'%(cid, k, k, cid), 
                                 "rainbow_cmap_fs",
                                 '%s:QPower'%(cid)+'_eq',
                                 "Q Power Scaled",
                                 '%s/Q:dfmux_samples'%(cid),
                                 display_in_info_bar = True
                             ))



        CC.addGlobalEquation(config_dic, 
                             CC.getEquation(
                                 ('* ! = %s:carrier_amplitude 0 ' +
                                 '%s/I:dfmux_samples')%(cid,cid), 
                                 "white_cmap",
                                 '%s:IScaling'%(cid)+'_eq',
                                 "I Scaling",
                                 '%s/I:dfmux_samples'%(cid),
                                 display_in_info_bar = False,
                                 color_is_dynamic = True,
                                        ))

        CC.addGlobalEquation(config_dic, 
                             CC.getEquation(
                                 ('* ! = %s:carrier_amplitude 0 ' +
                                 '%s/Q:dfmux_samples')%(cid,cid), 
                                 "white_cmap",
                                 '%s:QScaling'%(cid)+'_eq',
                                 "Q Scaling",
                                 '%s/Q:dfmux_samples'%(cid),
                                 display_in_info_bar = False,
                                 color_is_dynamic = True,
                                        ))




        CC.addGlobalEquation(config_dic, 
                             CC.getEquation('/ * %s:carrier_frequency = %s:carrier_frequency %s:demod_frequency %f'%(cid, cid, cid, max_freq), 
                                            "rainbow_cmap",
                                            '%s:freq'%(cid)+'_eq',
                                            "Frequency Settings",
                                            '%s:carrier_frequency'%(cid),
                                            display_in_info_bar = False,
                                        ))

        CC.addGlobalEquation(config_dic, 
                             CC.getEquation('/ %s:carrier_amplitude CarrierAmpMax'%(cid), 
                                            "rainbow_cmap",
                                            '%s:camp'%(cid)+'_eq',
                                            "Carrier Amplitude",
                                            '%s:carrier_amplitude'%(cid),
                                            display_in_info_bar = False,
                                        ))



        eqs_lst = ['%s:Rfractional'%(cid)+'_eq',
                   '%s:phase'%(cid)+'_eq',
                   '%s:freq'%(cid)+'_eq',
                   '%s:camp'%(cid)+'_eq',

                   '%s:SquidGood'%(mid)+'_eq',
                   
                   '%s:IPower'%(cid)+'_eq',
                   '%s:QPower'%(cid)+'_eq',

                   '%s:IScaling'%(cid)+'_eq',
                   '%s:QScaling'%(cid)+'_eq',
                   '%s/I:dfmux_samples_eq' % cid, 
                   '%s/Q:dfmux_samples_eq' % cid,
                   '%s:Resistance_eq' % cid ]


        #eqs_lst.append('%s:Resistance_eq'%cid)

        for mvs in get_module_vals():
            eqs_lst.append('%s:%s_eq' % (mid, mvs))
        for cvs in get_channel_vals():
            eqs_lst.append('%s:%s_eq' % (cid, cvs))
        for bvs in get_board_vals():
            eqs_lst.append('%s:%s_eq' % (bid, bvs))


        CC.addVisElem(config_dic, 
                      x_cen=fl(bp.x_offset * 10),   y_cen=fl(bp.y_offset * 10),
                      x_scale=scale_fac, y_scale=scale_fac, 
                      rotation=fl(bp.pol_angle),
                      svg_path=svg,
                      highlight_path = h_svg,
                      layer = 1, labels = [k, cid, bp.physical_name],
                      equations = eqs_lst, 
                      labelled_data = [ 
                          ["PhysId", bp.physical_name],
                          ["DevId", cid],
                          ["Board", bid],
                          ["Module", cid],
                      ],
                      group = group )


def generate_dfmux_lyrebird_config(fn, 
                                   wiring_map, bolo_props_map, 
                                   win_x_size = 800,
                                   win_y_size = 800,
                                   sub_sampling = 4,
                                   max_framerate = -1,
                                   max_num_plotted  = 10,
                                   hostname = '127.0.0.1',
                                   hk_hostname = '127.0.0.1',
                                   port = 8675, 
                                   hk_port = 8676,  
                                   control_host = None,
                                   gcp_get_hk_port = None,
                                   dv_buffer_size = 512,
                                   min_max_update_interval = 300,
                                   mean_decay_factor = 0.01
):
    import os
    creepy_path = os.path.dirname(os.path.realpath(__file__))
    svg_folder = os.path.abspath(creepy_path+'/../../svgs/') + '/'


    cell_size = 100
    safety_factor = 1.2
    min_delt = 1e-5

    board_ids = []
    #need to get boards list
    for k in wiring_map.keys():
        wm = wiring_map[k]
        board_ids.append( get_physical_id(wm.board_serial, wm.crate_serial, wm.board_slot) )
    board_ids = uniquifyList(board_ids)
    
    #need to get smallest spacing
    special_key = bolo_props_map.keys()[0]
    bp_0 = bolo_props_map[special_key]

    max_delt = 1e12
    for k in bolo_props_map.keys():
        bp =  bolo_props_map[k]
        delt = (fl(bp_0.x_offset) - fl(bp.x_offset))**2.0 + (fl(bp_0.y_offset)- fl(bp.y_offset))**2.0 
        if delt < max_delt and delt > min_delt:
            max_delt = delt
    special_separation = max_delt**0.5
    scale_fac =  special_separation / ( safety_factor * float(cell_size))

    global_display_names = ['Rfrac', 
                            'IQ Phase', 
                            'Freq Settings', 
                            'Carrier Amp',
                            'SQUID Be F*cked',
                            'I:HPF Power Units',
                            'Q:HPF Power Units',
                            'I:Dynamic Color Adjusted',
                            'Q:Dynamic Color Adjusted']

    #add the general settings
    config_dic = {}
    CC.addGeneralSettings(config_dic, 
                          win_x_size = win_x_size,
                          win_y_size = win_y_size,
                          sub_sampling = sub_sampling,
                          max_framerate= max_framerate,
                          max_num_plotted = max_num_plotted,
                          eq_names = global_display_names,
                          dv_buffer_size = dv_buffer_size,
                          min_max_update_interval = min_max_update_interval
                   )
    #import pdb; pdb.set_trace()
    addDfmuxStreamer(config_dic, "dfmux_streamer", board_ids, 
                     bolo_list = filter(lambda x: x, wiring_map.keys()),
                     sender_hostname = hk_hostname,
                     sender_port = port,
                     hk_hostname = hk_hostname,
                     hk_port = hk_port,
                     mean_decay_factor = mean_decay_factor
    )

    addDfmuxVisElems(config_dic, wiring_map, bolo_props_map, 
                     scale_fac, svg_folder)

    CC.addDataVal(config_dic, "PowScaling(fW)", 10, False)
    CC.addDataVal(config_dic, "CarrierAmpMax", 0.1, False)
    config_dic["modifiable_data_vals"] = ["PowScaling(fW)", 'CarrierAmpMax']

    if (not control_host is None) and (not gcp_get_hk_port is None):
        config_dic['external_commands_id_list'] = ['Get Housekeeping']
        config_dic['external_commands_list'] = ['nc -w 1 %s %d' % (control_host,gcp_get_hk_port)]

    CC.storeConfigFile(config_dic, fn) 

    
                           
