import argparse, json, os
import config_constructor as CC
import numpy as np
from operator import itemgetter, attrgetter

N_MODULES=8
N_CHANNELS=64

hostname = 'laphroaig.berkeley.edu'
port = 8675

def uniquifyList(seq):
    checked = []
    for e in seq:
        if e not in checked:
            checked.append(e)
    return checked


def addDataStreamer(config_dic, tag, boards_list, 
                    sender_hostname = 'laphroaig.berkeley.edu',
                    sender_port = 8675):

    desc = {'board_list':boards_list,
            'network_streamer_hostname': sender_hostname, 
            'network_streamer_port': sender_port }
    glob_eqs = []
    for b in boards_list:
        for m in range(N_MODULES):
            for c in range(N_CHANNELS):
                CC.addGlobalEquation(config_dic, 
                                     CC.getEquation('%s/%d/%d/I:dfmux_samples'%(b,m,c), 
                                                    'white_cmap', 
                                                    '%s/%d/%d/I:dfmux_samples'%(b,m,c)+'_eq',
                                                    'I_Sample'
                                                ))
                CC.addGlobalEquation(config_dic, 
                                     CC.getEquation('%s/%d/%d/Q:dfmux_samples'%(b,m,c), 
                                                    'white_cmap', 
                                                    '%s/%d/%d/Q:dfmux_samples'%(b,m,c)+'_eq',
                                                                'Q_Sample'
                                                ))
                glob_eqs.append('%s/%d/%d/Q:dfmux_samples'%(b,m,c)+'_eq')
    CC.addDataSource(config_dic, tag, 'dfmux',  desc)
    return glob_eqs

config_dic = {}
config_dic["displayed_global_equations"] = []

CC.addGeneralSettings(config_dic, win_x_size=800, win_y_size=600, sub_sampling=4, 
                   max_framerate=-1, max_num_plotted=10)

eqs = addDataStreamer(config_dic, 'dstreamer', boards_list = ['62_7'])
config_dic["displayed_global_equations"] = eqs


CC.addGlobalEquation(config_dic, CC.getEquation(".5", "red_cmap", "EqLabel", "DispEqLabel"))
CC.addVisElem(config_dic, 
              x_cen=0, y_cen=0, x_scale=1, y_scale=1, rotation=0, 
              svg_path= '/home/nlharr/spt_code/lyrebird/svgs/box.svg', 
              highlight_path = '/home/nlharr/spt_code/lyrebird/svgs/boxhighlight.svg',
              layer = 1, labels = ['VisElem'],
              equations = ["EqLabel"], 
              labelled_data = [], group = 'boxerson'   )

CC.storeConfigFile(config_dic, "test_dfmux_config.json")
