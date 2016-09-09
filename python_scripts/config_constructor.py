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

def addGeneralSettings(config_dic, win_x_size, win_y_size, sub_sampling, 
                       max_framerate, max_num_plotted, eq_names = []):
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
                                       'eq_names': eq_names
                              }    
def addDataVal(config_dic, dv_id, init_val, is_buffered):
    if not 'data_vals' in config_dic:
        config_dic['data_vals'] = []
    config_dic['data_vals'].append( (str(dv_id), float(init_val), bool(is_buffered)))


def addDataSource(config_dic, tag, ds_type, desc, update_time=1):
    if not 'data_sources' in config_dic:
        config_dic['data_sources'] = []
    config_dic['data_sources'].append( {'tag':tag,
                                        'ds_type':ds_type,
                                        'update_time':update_time,
                                        'desc':desc})

def getEquation(eq_func, eq_color_map, eq_label, display_label, sample_rate_val):
    return {"function":eq_func,"cmap": eq_color_map, "label": eq_label, 
            "display_label":display_label, "sample_rate_id":sample_rate_val}

def addGlobalEquation(config_dic, equation):
    if not 'equations' in config_dic:
        config_dic['equations'] = []
    config_dic['equations'].append(equation)


def addVisElem(config_dic, 
               x_cen, y_cen, x_scale, y_scale, rotation, 
               svg_path, highlight_path,
               layer, labels,
               equations, 
               labelled_data, group
           ):

    assert(len(labels))
    assert(len(equations))

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

    scale_factor = 0.008

    color_maps = ['bolo_cyan_cmap', 'bolo_green_cmap','bolo_blue_cmap','white_cmap']
    
    #addDataSource(config_dic, "test_ds_file_2", "test_ds_2_global", "test_global_id", "test_streamer", "streaming", False)

    addDataVal(config_dic, "test_global_id", 42, False)
    addGlobalEquation(config_dic, getEquation("a test_global_id",  "red_cmap", "GlobalEqLabel", "GlobalEquation", "test_global_id"));

    nis = 50
    njs = 50
    npol = 2
    nfre = 3

    svg_folder = os.path.abspath('../svgs')+'/'

    test_ds_lst = []
    for p in range(npol):
        for f in range(nfre):
            for i in range(nis):
                for j in range(njs):

                    ds_id_num = (i*njs*npol*nfre + j * npol * nfre + p*nfre + f)

                    cmap = color_maps[f]

                    extra_pos = 0.2

                    #addDataVal(config_dic, "test_%d"%ds_id_num, 0, True)
                    test_ds_lst.append("test_%d"%ds_id_num)

                    addGlobalEquation(config_dic, getEquation('/ + 1 c test_%d 2'%ds_id_num, cmap, "dummyEqLabel_test_%d"%ds_id_num, "TestSins", 'test_%d'%ds_id_num))
                    addGlobalEquation(config_dic, getEquation('test_%d'%ds_id_num, cmap, "dummyLinearEq_test_%d"%ds_id_num, "TestLins", 'test_%d'%ds_id_num,))

                    if f ==0:
                        svg_name = svg_folder + 'smallpol.svg'
                        svg_h = svg_folder + 'smallhighlight.svg'
                    elif f == 1:
                        svg_name = svg_folder + 'medpol.svg'
                        svg_h = svg_folder + 'medhighlight.svg'
                    else:
                        svg_name = svg_folder + 'largepol.svg'
                        svg_h = svg_folder + 'largehighlight.svg'
                    addVisElem(config_dic, 
                               x_cen=i, y_cen=j, 
                               x_scale=scale_factor, y_scale=scale_factor, 
                               rotation=1.5707963267948966*p, layer = 1-f,
                               svg_path=svg_name,
                               highlight_path = svg_h,
                               
                               labels=['test_label_%d'%ds_id_num, 'test_sec_label_%d'%ds_id_num],
                               group = 'Detector_type_%d'%f,
                               equations = ["dummyEqLabel_test_%d"%ds_id_num, "dummyLinearEq_test_%d"%ds_id_num  ],
                               labelled_data=[["hey you","let's dance"], ["no seriously","dance"]]
                    )
    config_dic["displayed_global_equations"] = ["dummyEqLabel_test_0", "dummyEqLabel_test_6"]
    config_dic["modifiable_data_vals"] = ["test_global_id"]

    config_dic['external_commands_list'] = ['echo hello', 'echo goodbye']
    config_dic['external_commands_id_list'] = ['SAY HALLO', "SAY GOODBYE"]



    addDataSource(config_dic, "test_streamer", "test_streamer", test_ds_lst, 1000)
    storeConfigFile(config_dic, "test_config_file.json")

    
