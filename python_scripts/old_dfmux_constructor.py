import argparse, json, os
import config_constructor as CC
import numpy as np
from operator import itemgetter, attrgetter
N_MODULES=8
N_CHANNELS=64





'''
- Need list of physical ids we want information for
- Need mapping from physical id to stored id.
- Need pixel ids



test_channel_format = {'Sq2SB21Ch3':  
                        {'module': 2,
                         'channel':3,
                         'board':'iceboard0043.local',
                         'other_ids': [],
                         'x_pos':3.2,
                         'y_pos':4.2,
                         'is_polarized': True,
                         'polarization_angle': .1,
                         'resistance': 32132.3,
                         'detector_frequency': #90,150, or 220
                         'pixel_id': 'pixelid'
                         }
                       }
'''
def uniquifyList(seq):
    checked = []
    for e in seq:
        if e not in checked:
            checked.append(e)
    return checked

def addHousekeeping(config_dic, tag, boards_list, include_self_equations = True):
    #this needs to match the C code, don't change this expecting more data to appear
    MODULE_DATA_NAMES = ['carrier_gain', 'nuller_gain']
    CHANNEL_DATA_NAMES = ['carrier_amplitude', 'carrier_frequency', 'demod_frequency']
    for b in boards_list:
        for m in range(N_MODULES):
            for mdn in MODULE_DATA_NAMES:
                dvname = '%s/%d:%s'%(b,m,mdn)
                CC.addDataVal(config_dic, dvname,-1,False)
                if include_self_equations:
                    CC.addGlobalEquation(config_dic, CC.getEquation(dvname, 'white_cmap',  dvname+'_eq', mdn))
            for c in range(N_CHANNELS):
                for cdn in CHANNEL_DATA_NAMES:
                    dvname = '%s/%d/%d:%s'%(b,m,c,cdn)
                    CC.addDataVal(config_dic, dvname,-1,False)
                    if include_self_equations:
                        CC.addGlobalEquation(config_dic, CC.getEquation(dvname, 'white_cmap',  dvname+'_eq', cdn))
    desc = { 'hostnames': boards_list}
    CC.addDataSource(config_dic, tag, 'housekeeping',  desc)

def addDfmux(config_dic, tag, boards_list, include_self_equations = True, listen_ip_address = "192.168.1.227", n_boards = 12):
    for b in boards_list:
        for m in range(N_MODULES):
            for c in range(N_CHANNELS):
                CC.addDataVal(config_dic, '%s/%d/%d/I:dfmux_samples'%(b,m,c),-1,True)
                CC.addDataVal(config_dic, '%s/%d/%d/Q:dfmux_samples'%(b,m,c),-1,True)
                if include_self_equations:
                    CC.addGlobalEquation(config_dic, CC.getEquation('%s/%d/%d/I:dfmux_samples'%(b,m,c), 'white_cmap', 
                                                                    '%s/%d/%d/I:dfmux_samples'%(b,m,c)+'_eq',
                                                                    'I_Sample'
                                                                ))
                    CC.addGlobalEquation(config_dic, CC.getEquation('%s/%d/%d/Q:dfmux_samples'%(b,m,c), 'white_cmap', 
                                                                    '%s/%d/%d/Q:dfmux_samples'%(b,m,c)+'_eq',
                                                                    'Q_Sample'
                                                                ))
    desc = { 'hostnames': boards_list, "listen_ip_address":listen_ip_address, "n_boards":n_boards}
    CC.addDataSource(config_dic, tag, 'dfmux',  desc)


def create_dfmux_config_dic( config_dic, bolo_description_dic, 
                             bolo_svg_scale_factor = 0.01, 
                             svg_folder = os.path.abspath('../svgs')+'/'):
    boards_list = uniquifyList(map(lambda k: bolo_description_dic[k]['board'], bolo_description_dic))
    '''
    pixel_list = uniquifyList(map(lambda k: bolo_description_dic[k]['pixel_id'], bolo_description_dic))
    pixel_map = {}
    for p in pixel_list:
        pixel_map[p] = False
    '''

    #figures out the scale factor we want
    xs = np.array(map(lambda k: bolo_description_dic[k]['x_pos'], bolo_description_dic))
    ys = np.array(map(lambda k: bolo_description_dic[k]['y_pos'], bolo_description_dic))

    CC.addGeneralSettings(config_dic, win_x_size=800, win_y_size=600, sub_sampling=4, max_framerate=20, max_num_plotted=20)
    addHousekeeping(config_dic, "Housekeeping", boards_list)
    addDfmux(config_dic, "Dfmux", boards_list)

    for k in bolo_description_dic:
        v = bolo_description_dic[k]
        board = v['board']
        module = v['module']
        channel = v['channel']
        x = v['x_pos']
        y = v['y_pos']
        resistance = v['resistance']
        detector_frequency = v['detector_frequency']
        is_polarized = v['is_polarized']
        polarization_angle = v['polarization_angle']
        other_ids = v['other_ids']
        if is_polarized:
            if detector_frequency == 90:
                svg_path = svg_folder + '/largepol.svg'
                highlight_path = svg_folder + '/largehighlight.svg'
                cmap = 'bolo_green_cmap'
            elif detector_frequency == 150:
                svg_path = svg_folder + '/medpol.svg'
                highlight_path = svg_folder + '/medhighlight.svg'
                cmap = 'bolo_blue_cmap'
            elif detector_frequency == 220:
                svg_path = svg_folder + '/smallpol.svg'
                highlight_path = svg_folder + '/smallhighlight.svg'
                cmap = 'bolo_purple_cmap'
            else:
                print "I don't recognize your detector frequency"
                svg_path = svg_folder + '/box.svg'
                highlight_path = svg_folder + '/boxhighlight.svg'
        else:
            svg_path = svg_folder + '/box.svg'
            highlight_path = svg_folder + '/boxhighlight.svg'
            cmap = 'bolo_blue_cmap'


        '''
        - Slow Changing Detector
        - ask for squid health
        - get label of the squid
        '''


        cgain_str = "%s/%d:carrier_gain" %(board, module)
        ngain_str = "%s/%d:nuller_gain" % (board, module)
        camp_str = "%s/%d/%d:carrier_amplitude" % (board, module, channel)
        namp_str = "%s/%d/%d/I:dfmux_samples" %(board, module, channel)


        test_eq = CC.getEquation("- / %s 1000 4"%(namp_str),
                                cmap, 
                                "%s/%d/%d:test_eq"%(board, module, channel), 
                                "TEST")
        CC.addGlobalEquation(config_dic, test_eq)


        res_eq = CC.getEquation("/ r %s %s %s %s %f"%(camp_str, cgain_str, namp_str, ngain_str, resistance),
                                cmap, 
                                "%s/%d/%d:rfrac_eq"%(board, module, channel), 
                                "RFrac")
        CC.addGlobalEquation(config_dic, res_eq)
        equations = [
            "%s/%d/%d:test_eq"%(board, module, channel),
            "%s/%d/%d:rfrac_eq"%(board, module, channel),
            "%s/%d:carrier_gain_eq" %(board, module),
            "%s/%d:nuller_gain_eq" % (board, module),
            "%s/%d/%d:carrier_amplitude_eq" % (board, module, channel),
            "%s/%d/%d:carrier_frequency_eq" % (board, module, channel),
            "%s/%d/%d/I:dfmux_samples_eq" % (board, module, channel),
            "%s/%d/%d/Q:dfmux_samples_eq" % (board, module, channel),
        ]
        #creates all the equations we want:
        #carrier frequency #carrier gain
        #nuller gain  #current r frac value
        CC.addVisElem(config_dic, 
                      x_cen = x, y_cen = y, 
                      x_scale = bolo_svg_scale_factor, y_scale = bolo_svg_scale_factor, 
                      rotation = polarization_angle,
                      svg_path = svg_path,
                      highlight_path = highlight_path,
                      layer = 1, labels= [k]+other_ids,
                      equations = equations,
                      labelled_data={}, 
                      group="%d_GHz_Detectors" % detector_frequency)


test_dfmux_desc = {'Sq1Ch1':  
                        {'module': 0,
                         'channel':0,
                         'board':'iceboard0043.local',
                         'other_ids': [],
                         'x_pos':0,
                         'y_pos':0,
                         'is_polarized': False,
                         'polarization_angle': 0,
                         'resistance': 4500.0,
                         'detector_frequency': 0,
                         'pixel_id': 'pixelid'
                         }
                       }

config_dic = {}
create_dfmux_config_dic(config_dic, test_dfmux_desc)

json.dump( config_dic, open("test_dfmux_display.json",'w'), indent=2)
