import json, os

def static_var(varname, value):
    def decorate(func):
        setattr(func, varname, value)
        return func
    return decorate

@static_var('paths', {})
def convert_svg_path_to_id(p):
    p = p.strip()
    assert(p[-4:] == '.svg')
    p = p[:-4]
    if p in convert_svg_path_to_id.paths:
        return convert_svg_path_to_id.paths[p]
    test_id = os.path.basename(p)
    i = 1
    while (test_id in convert_svg_path_to_id.paths.values()):
        test_id =  os.path.basename(p)+'_%d'%i
    convert_svg_path_to_id.paths[p] = test_id
    return test_id

def addGeneralSettings(config_dic, win_x_size, win_y_size, sub_sampling, max_framerate, max_num_plotted):
    assert(win_x_size > 0)
    assert(win_y_size > 0)
    assert(sub_sampling%2==0)
    assert(sub_sampling < 18)
    assert(max_num_plotted > 0)

    config_dic['general_settings'] =  {'win_x_size': win_x_size,
                                       'win_y_size': win_y_size,
                                       'sub_sampling': sub_sampling,
                                       'max_framerate': max_framerate,
                                       'max_num_plotted': max_num_plotted,
                                  }
    
def add_mod_data_val(config_dic, tag, val):
    if ('modifiable_dvs' not in config_dic):
        config_dic['modifiable_dvs'] = []
    config_dic['modifiable_dvs'].append({'tag':tag,'val':val})




def addDataSource(config_dic, ds_file, ds_path, ds_id, streamer_type, sampling_type, is_buffered):
    '''
    ds_id: is the label in the data source
    '''
    if not 'data_streams' in config_dic:
        config_dic['data_streams'] = []
    config_dic['data_streams'].append({'id': ds_id,
                                       'file': ds_file,
                                       'path': ds_path,
                                       'streamer_type':streamer_type,
                                       'sampling_type':sampling_type,
                                       'is_buffered': bool(is_buffered)
                                   })

def addConstVals(config_dic, constant_vals):
    if not 'constant_values' in config_dic:
        config_dic['constant_values'] = {}
    config_dic['constant_values'].update(constant_vals)


def getEquation(eq_func, eq_vars, eq_color_map, eq_label):
    return {"function":eq_func, "eq_vars":eq_vars, "cmap": eq_color_map, "label": eq_label}


def addGlobalEquation(config_dic, equation):
    if not 'global_equations' in config_dic:
        config_dic['global_equations'] = []
    config_dic['global_equations'].append(equation)


def addVisElem(config_dic, 
               x_cen, y_cen, x_scale, y_scale, rotation, 
               svg_path, highlight_path,
               layer, labels,
               constant_vals, equations, 
               labelled_data, group
           ):
    addConstVals(config_dic, constant_vals)

    if not 'visual_elements' in config_dic:
        config_dic['visual_elements'] = []

    svg_id = convert_svg_path_to_id(svg_path)
    highlight_id = convert_svg_path_to_id(highlight_path)

    config_dic['visual_elements'].append(
        {'x_center':x_cen,
         'y_center':y_cen,
         'x_scale':x_scale,
         'y_scale':y_scale,
         'rotation': rotation,
         'layer':layer,

         'svg_path':svg_path,
         'svg_id':convert_svg_path_to_id(svg_path),
         'highlight_svg_path':highlight_path,
         'highlight_svg_id':convert_svg_path_to_id(highlight_path),
         
         'equations':equations,
         'labels':labels,
         'group': group,
         'labelled_data': labelled_data,
     })



def validateConfigDic(config_dic):
    return True

def storeConfigFile(config_dic, fn):
    if (not validateConfigDic(config_dic)):
        raise RuntimeError("Attempting to store invalid configuration dictionary")

    f = open(fn, 'w')
    f.write( json.dumps(config_dic, indent=2))
    f.close()


if __name__ == '__main__':
    config_dic = {}
    addGeneralSettings(config_dic, win_x_size=800, win_y_size=600, sub_sampling=4, max_framerate=-1, max_num_plotted=10)

    scale_factor = 0.01

    constant_vals = {"const_val_0": 1, "const_val_1": 2 }
    addConstVals(config_dic, constant_vals)

    color_maps = ['red_cmap','green_cmap','blue_cmap','white_cmap']

    addDataSource(config_dic, "test_ds_file_2", "test_ds_2_global", "test_global_id", "test_streamer", "streaming", False)
    addGlobalEquation(config_dic, getEquation("a test_global_id", ["test_global_id"], "cmap_red", "Global Eq Label"));

    nis = 50
    njs = 50
    npol = 2
    nfre = 3


    svg_folder = os.path.abspath('../svgs')+'/'

    for p in range(npol):
        for f in range(nfre):
            for i in range(nis):
                for j in range(njs):

                    ds_id_num = (i*njs*npol*nfre + j * npol * nfre + p*nfre + f)

                    cmap_num = ds_id_num%len(color_maps)
                    cmap = color_maps[f]
                    sf = scale_factor*(.175 + .25*f )

                    extra_pos = 0.2

                    addDataSource(config_dic, "test_ds_file", "test_ds_path_%d"%(ds_id_num), "test_%d"%ds_id_num, "test_streamer", "streaming", True)
                    #{"equation":'cos(x)^2', 'eq_vars':['test_%d'%ds_id_num]}, color_map=cmap,
                    addVisElem(config_dic, 
                               x_cen=i*(1+extra_pos) + p, y_cen=j*(1+extra_pos), 
                               x_scale=sf, y_scale=scale_factor, 
                               rotation=1.5707963267948966*p, layer = 1-f,
                               svg_path=svg_folder + 'cross.svg', 
                               highlight_path = svg_folder + 'highlight.svg',


                               labels=['test_label_%d'%ds_id_num, 'test_sec_label_%d'%ds_id_num],
                               group = 'Detector_type_%d'%f,
                               equations = [getEquation('c test_%d'%ds_id_num, ['test_%d'%ds_id_num], cmap, "dummyEqLabel"),
                                            getEquation('a test_%d'%ds_id_num, ['test_%d'%ds_id_num], cmap, "dummyLinearEq"),
                                             ],
                               labelled_data={}, 
                               constant_vals={}                               
                    )

    storeConfigFile(config_dic, "test_config_file.json")

    
