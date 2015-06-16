import argparse, json, os
import config_constructor as CC

import numpy as np
from operator import itemgetter, attrgetter


N_MODULES=8
N_CHANNELS=64

'''
test_format = {'Sq2SB21Ch3':  {'module': 2,
               'channel':3,
'board':'iceboard0043.local',
'x_pos':3.2,
'y_pos':4.2,
       'is_polarized': True,
       'polarization_angle': .1,
       'resistance': 32132.3,
'detector_frequency': #90,150, or 220
                              }}
'''
def uniquifyList(seq):
    checked = []
    for e in seq:
        if e not in checked:
            checked.append(e)
    return checked

def addHousekeeping(config_dic, tag, boards_list, include_self_equations = False):
    #this needs to match the C code, don't change this expecting more data to appear
    MODULE_DATA_NAMES = ['carrier_gain', 'nuller_gain']
    CHANNEL_DATA_NAMES = ['carrier_amplitude', 'carrier_frequency', 'demod_frequency']
    for b in boards_list:
        for m in range(N_MODULES):
            for mdn in MODULE_DATA_NAMES:
                dvname = '%s/%d:%s'%(b,m,mdn)
                CC.addDataVal(config_dic, dvname,-1,False)
                if include_self_equations:
                    CC.addGlobalEquation(config_dic, CC.getEquation(dvname, 'cmap_white', 
                                                                 dvname+'_eq'))
            for c in range(N_CHANNELS):
                for cdn in CHANNEL_DATA_NAMES:
                    dvname = '%s/%d/%d:%s'%(b,m,c,cdn)
                    CC.addDataVal(config_dic, dvname,-1,False)
                    if include_self_equations:
                        CC.addGlobalEquation(config_dic, CC.getEquation(dvname, 'cmap_white', 
                                                                            dvname+'_eq'))
    desc = { 'hostnames': boards_list}
    CC.addDataSource(config_dic, tag, 'housekeeping',  desc)

def addDfmux(config_dic, tag, boards_list, include_self_equations = False):
    for b in boards_list:
        for m in range(N_MODULES):
            for c in range(N_CHANNELS):
                CC.addDataVal(config_dic, '%s/%d/%d/I:dfmux_samples'%(b,m,c),-1,False)
                CC.addDataVal(config_dic, '%s/%d/%d/Q:dfmux_samples'%(b,m,c),-1,False)
                if include_self_equations:
                    CC.addGlobalEquation(config_dic, CC.getEquation('%s/%d/%d/I:dfmux_samples'%(b,m,c), 'cmap_white', 
                                                                    '%s/%d/%d/I:dfmux_samples'%(b,m,c)+'_eq'))
                    CC.addGlobalEquation(config_dic, CC.getEquation('%s/%d/%d/Q:dfmux_samples'%(b,m,c), 'cmap_white', 
                                                                    '%s/%d/%d/Q:dfmux_samples'%(b,m,c)+'_eq'))
    desc = { 'hostnames': boards_list}
    CC.addDataSource(config_dic, tag, 'dfmux',  desc)


def create_dfmux_config_dic( config_dic, bolo_description_dic):
    boards_list = uniquifyList(map(lambda k: bolo_description_dic[k]['board'], bolo_description_dic))
    #figures out the scale factor we want
    xs = np.array(map(lambda k: bolo_description_dic[k]['x_pos'], bolo_description_dic))
    ys = np.array(map(lambda k: bolo_description_dic[k]['y_pos'], bolo_description_dic))


    CC.addGeneralSettings(config_dic, win_x_size=800, win_y_size=600, sub_sampling=4, max_framerate=40, max_num_plotted=20)
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








boards_list = ['iceboard0043.local']
modules_list = [1,2,3,4]

config_dic = {}
svg_folder = os.path.abspath('../svgs')+'/'
CC.addGeneralSettings(config_dic, win_x_size = 800, win_y_size=600, 
                   sub_sampling=2, max_framerate=60, max_num_plotted=10)

                                        
addHousekeeping(config_dic, "Housekeeping", boards_list)

CC.addVisElem(config_dic, 
           x_cen=0, y_cen=0, x_scale=1, y_scale=1, rotation=0, 
           svg_path=svg_folder + 'box.svg', 
           highlight_path = svg_folder + 'highlight.svg',
           layer = 1, labels=["testbox"],
           equations = ["iceboard0043.local/1:carrier_gain_eq"], 
           labelled_data={}, group="Test")


#print json.dumps(config_dic, indent = 2)
json.dump( config_dic, open("test_eq_display.json",'w'), indent=2)
