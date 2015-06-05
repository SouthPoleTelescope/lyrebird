import argparse, json, tables, os, sys
import numpy as np
import pylab as pl
import config_constructor as CC

##########################################                                                      
def prep_legendre(n,polyorder):
    '''make array of legendre's'''
    legendres=np.empty([n,polyorder+1])
    legendres[:,0]=np.ones(n)
    if polyorder > 0:
        legendres[:,1]=np.linspace(-1,1,n)
    for i in range(polyorder-1):
        l = i+1
        legendres[:,l+1]=((2*l+1)*legendres[:,1]*legendres[:,l]-l*legendres[:,l-1])/(l+1)

    q,r=np.linalg.qr(legendres)
    rinv = np.linalg.inv(r)
    qt=q.T.copy()
    return legendres,rinv,qt

##########################################################################################################
def poly_legendre_qr_mask_precalc(bolo,mask,legendres):
        m=legendres.shape[1]
        n=legendres.shape[0]
        l2 = legendres*np.tile(mask.reshape(n,1),[1,m])
        q,r=np.linalg.qr(l2)

        rinv = np.linalg.inv(r)
        p = np.dot(q.T,bolo)
        coeff=np.dot(rinv,p)
        out=bolo-np.dot(legendres,coeff)
        return out

##########################################################################################################
def poly_legendre_qr_nomask_precalc_inplace(bolo,legendres,rinvqt):
        coeff = np.dot(rinvqt,bolo)
        bolo -= np.dot(legendres,coeff)

##########################################################################################################
def applyPolyFilterArray(array,mask,polyorder):
    """ writes over input array                                                                           
    CR                                                                                                     
    """
    success=1
    if polyorder < 0:
        return success,array
    #damn, work                                                                                            
    nch=array.shape[0]
    nt=array.shape[1]

    #remove mean                                                                                           
    if polyorder == 0:
        for i in range(nch):
            if np.any(mask[i,:]):
                array[i,:] -= np.average(array[i,:],weights=mask[i,:])
    #other cases                                                                                           
    if polyorder > 0:
        legendres,rinv,qt=prep_legendre(nt,polyorder)
        rinvqt = np.dot(rinv,qt)
        goodhits = np.sum(mask[:,:],axis=1)
        for i in xrange(nch):
            if goodhits[i] != nt:
                continue #skip for now                               
            poly_legendre_qr_nomask_precalc_inplace(
                array[i,:],legendres,rinvqt)
        for i in xrange(nch):
            if ((goodhits[i] == nt) or (goodhits[i] == 0)):
                continue #skip since dealt with above, or we're skipping it 
            if goodhits[i] < (polyorder+1)*2: #not enough points  
                print i,goodhits[i],(polyorder+1)*2
                success=0
                return success,array
            array[i,:] = poly_legendre_qr_mask_precalc(
                array[i,:], mask[i,:],legendres)
    return success,array

#########################################

def filter_tod_for_lyrebird( array, scan_start_inds, scan_stop_inds, polyorder):
    print 'poly filtering'
    array[:, :scan_start_inds[0]] = 0
    array[:, scan_stop_inds[-1]:] = 0

    #zero portions between scans
    for i in xrange(1, len(scan_start_inds)):
        array[:, scan_stop_inds[i-1]:scan_start_inds[i]] = 0

    #poly filters on the scans
    for i in xrange(len(scan_start_inds)):
        print 'on scan %d'%i
        start = scan_start_inds[i]
        stop  = scan_stop_inds[i]
        sub_arr = array[:, start:stop]
        out_arr = applyPolyFilterArray(  sub_arr, sub_arr*0+1, polyorder)
    return array

def is_150(fn):
    return fn.find('150ghz') != -1

import warnings; warnings.filterwarnings('ignore', category=tables.NaturalNameWarning)

file_names = [
    #'/home/nlharr/tmp/rcw38_idf_20130901_213033_150ghz.h5',
    '/home/nlharr/tmp/ra0hdec-57.5_idf_20131101_020307_150ghz.h5',
    #'/home/nlharr/tmp/ra0hdec-57.5_idf_20131101_020307_090ghz.h5',

             ]

polyorder = 4
output_hdf_file = 'test_hdf_output.hdf5'
output_config_file = 'test_hdf_config.json'
#skip_hdf = False
skip_hdf = True
#  0.5 * (r - rs)
color_map_to_use = 'rainbow_cmap'
scale_factor = .0125
#scale_factor = .001

###########################################
svg_folder = os.path.abspath('../svgs')+'/'
config_dic = {}
CC.addGeneralSettings(config_dic, win_x_size=800, win_y_size=600, sub_sampling=2, max_framerate=100, max_num_plotted=100)
CC.add_mod_data_val(config_dic, "scale_factor", scale_factor)

if not skip_hdf:
    outf = tables.openFile(output_hdf_file, 'w')

for fn in file_names:
    f = tables.openFile(fn)
    #loads the parameters for the lyrebird config
    xoff = f.getNode('/data/detector_parameters/pointing_offset_x').read()
    yoff = f.getNode('/data/detector_parameters/pointing_offset_y').read() * -1 #something with dec el
    pang = f.getNode('/data/detector_parameters/pol_angle').read()
    bids = f.getNode('/data/observation/bolo_id_ordered').read()

    if is_150(fn):
        image_scale_factor = .0125
    else:
        image_scale_factor = .025

    for i in xrange(len(bids)):
        frot = float(pang[i])
        rot_cor_x = 0
        rot_cor_y = 0

        CC.addDataSource(config_dic, ds_file=output_hdf_file, ds_path=bids[i], ds_id='r'+bids[i], 
                      streamer_type="hdf_history", sampling_type="request_history")
        CC.addVisElem(config_dic, 
                   x_cen=float(xoff[i]) + rot_cor_x, y_cen=float(yoff[i]) + rot_cor_y, 
                   x_scale=image_scale_factor*.8, y_scale=image_scale_factor, 
                   rotation = frot,
                   svg_path=svg_folder + 'cross.svg', 
                   highlight_path = svg_folder + 'highlight.svg',
                   layer = 2, labels = [bids[i]],
                   constant_vals={}, 
                   equations = [
                       CC.getEquation(eq_func='+ / * 0.5 r%s scale_factor 0.5'%(bids[i]), eq_vars=[], 
                                   eq_color_map = color_map_to_use, eq_label = 'PlotResp'),

                       CC.getEquation(eq_func='%f'%(frot), eq_vars=[], 
                                      eq_color_map = color_map_to_use, eq_label = 'PolRot'),

                       CC.getEquation(eq_func='%f'%(float(xoff[i])), eq_vars=[], 
                                      eq_color_map = color_map_to_use, eq_label = 'XPos'),
                       CC.getEquation(eq_func='%f'%(float(yoff[i])), eq_vars=[], 
                                      eq_color_map = color_map_to_use, eq_label = 'YPos'),
                   ], 
                   labelled_data={}, group = '')

    #filters our array
    if not skip_hdf:
        bdarray = f.getNode('/data/bolodata_array').read()
        sstart = f.getNode('/data/scan/start_index').read()
        sstop = f.getNode('/data/scan/stop_index').read()
        bdarray = filter_tod_for_lyrebird(bdarray, sstart, sstop, polyorder)
        for i in xrange(len(bids)):
            outf.create_array('/',bids[i], bdarray[i,:])
    f.close()

if not skip_hdf:
    outf.close()
    

#import pprint; pprint.pprint( config_dic)
#print json.dumps(config_dic)
json.dump(config_dic, open( output_config_file, 'w'), indent = 2)
os.system("../src/lyrebird %s"%output_config_file)

