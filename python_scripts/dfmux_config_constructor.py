import argparse, json, os
import config_constructor as CC


# things we need

def addHousekeeping(config_dic, tag, boards_list, include_self_equations = True):
    N_MODULES=8
    N_CHANNELS=64

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
