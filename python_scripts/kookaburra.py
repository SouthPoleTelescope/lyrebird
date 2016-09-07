import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import socket, curses, json, traceback, math
from dfmux_config_constructor import get_physical_id, sq_phys_id_to_info

'''
needs channel list
squids list
'''

def generate_kookie_config_file(output_file, 
                                squids_list,
                                squid_labels_list,
                                channels_list,
                                listen_hostname = '127.0.0.1',
                                listen_port = 8675):
    out_d = {}
    out_d['squids_list'] = squids_list
    out_d['squid_labels_list'] = squid_labels_list
    out_d['channels_list'] = channels_list
    out_d['listen_hostname'] = listen_hostname
    out_d['listen_port'] = listen_port
    json.dumps(out_d, open(output_file, 'w'))

def read_kookie_config_file(fn):
    d = json.load(open(fn))
    return d

class IdIpMapper(object):
    def __init__(self, wiring_map):
        self.mp = {}
        for k in wiring_map.keys():
            board_id = get_physical_id(wm.board_serial,
                                       wm.crate_serial,
                                       wm.board_slot)
            self.mp[ wm.ip ] = board_id
            self.mp_inv[board_id] = wm.ip
    def get_id(ip):
        return self.mp[ip]
    def get_ip(id):
        return self.mp_inv[id]


class FancyScatterPlot(object):
    def __init__(self, x, y, labels, 
                 x_label = 'Test X Label',
                 y_label = 'Test Y Label',
                 send_port = 5555,
                 recv_port = 5556,
                 frac_screen = 0.03):
        plt.ion()
        self.set_labels(labels)
        self.x = x
        self.y = y
        self.frac_screen = frac_screen
        self.figure = plt.figure()
        self.gs = gridspec.GridSpec(3, 3)
        ax1 = plt.subplot(self.gs[0:2,0:2])
        self.ax = ax1
        self.main_plot, = plt.plot(x, y, '*')
        self.overplot, = plt.plot([],[], 'ro')
        self.x_label = x_label
        self.y_label = y_label
        plt.ylabel(self.y_label)

        self.update_hists()

        self.figure.canvas.mpl_connect('button_press_event', self.mycall)
        self.highlighted = []

        UDP_IP = "127.0.0.1"
        UDP_PORT = 5556
        self.sock_listen = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.sock_listen.setblocking(False)
        self.sock_listen.bind((UDP_IP, UDP_PORT))
        self.sock_send = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.send_port = send_port
        plt.show()

    def check_socket(self):
        labels = []
        while 1:
            try:
                data = self.sock_listen.recv(1024)
                labels.append(data[:-1])
            except:
                break
        if len(labels) > 0:
            self.highlight_strs(labels)
            
    def set_labels(self, labels):
        self.labels = labels
        self.label_map = {}
        for i in range(len(labels)):
            self.label_map[labels[i]] = i

    def update_hists(self):
        ax2 = plt.subplot(self.gs[2,0:2], sharex=self.ax)
        self.x_hist = plt.hist(self.x)
        plt.xlabel(self.x_label)
        ax2 = plt.subplot(self.gs[0:2,2], sharey=self.ax)
        self.y_hist = plt.hist(self.y, orientation = 'horizontal')

    def update_data(self, x,y):
        self.x = x
        self.y = y
        self.main_plot.set_xdata(self.x)
        self.main_plot.set_ydata(self.y)
        self.update_hists()
        self.highlight_inds(self.highlighted, no_send = True)

    def highlight_inds(self, valid_inds, no_send = True):
        self.highlighted = valid_inds

        if len(valid_inds) > 0:
            for i in valid_inds:
                self.overplot.set_xdata( x[valid_inds] )
                self.overplot.set_ydata( y[valid_inds] )
        else:
            self.overplot.set_xdata( [] )
            self.overplot.set_ydata( [] )
        self.figure.canvas.draw()
        if not no_send:
            for i in valid_inds:
                self.sock_send.sendto(labels[i], ('localhost', self.send_port))

    def highlight_strs(self, strs, no_send = True):
        inds = []
        for s in strs:
            if s in self.label_map:
                inds.append(self.label_map[s])
        self.highlight_inds(inds, no_send = True)
            
    def mycall(self, event):
        self.event = event
        xsize = self.ax.get_xlim()[1]-self.ax.get_xlim()[0]
        ysize = self.ax.get_ylim()[1]-self.ax.get_ylim()[0]
        dist_perc =(((self.x-event.xdata)/xsize)**2 + ((self.y-event.ydata)/ysize)**2)**.5
        valid_inds = np.where( dist_perc < self.frac_screen)[0]
        self.highlight_inds(valid_inds, no_send = False)


class FocalPlaneStater(object):
    def __init__(self, channels_lst, 
                 noise_buffer_size = 128,
                 update_modulo = 1
    ):
        self.channels_lst = channels_lst
        self.channel_map = {}
        for i, ch in enumerate(channels_lst):
            self.channel_map[ch] = i

        self.noise_buffers = np.array([np.zeros( noise_buffer_size ) for ch in channels_lst])
        self.current_val = np.array([0 for ch in channels_lst])
        self.rnorm_conv = [1 for ch in channels_lst]

        self.noise_buffer_ind = 0
        self.noise_buffer_size = noise_buffer_size
        self.id_ip_mapper = None

        self.update_modulo = update_modulo
        self.update_ind = 0
        
        self.plotter = FancyScatterPlot( x = self.current_val, 
                                         y = np.std(self.noise_buffers, axis =1),
                                         labels = self.channels_lst, 
                                         x_label = '$R_{frac}$',
                                         y_label = 'Noise' )
        plt.show(block = False)
    def __call__(self, frame):
        if frame.type == core.G3FrameType.Wiring:
            self.id_ip_mapper = IdIpMapper(frame['WiringMap'])
        elif frame.type == core.G3FrameType.Timepoint:
            if self.id_ip_mapper == None:
                return
            meta_samp = frame['DfMux']
            for ip in meta_samp.keys():
                board_id = self.id_ip_mapper.get_id(ip)
                for module_id in meta_samp[ip].keys():
                    samp = meta_samp[ip][module_id]
                    for i in range(len(samp)):
                        cid = '%s/%d/%d' % (board_id, module_id, i)
                        if cid in self.channel_map:
                            ind = self.channel_map[cid]
                            sval = samp[i*2]
                            self.noise_buffers[ind][self.noise_buffer_ind] = sval
                            self.current_val[ind] = sval
            self.noise_buffer_ind = (self.noise_buffer_ind + 1) % self.noise_buffer_size
            if self.update_ind % self.update_modulo == 0:
                self.plotter.update_data(self.current_val, np.std(self.noise_buffers, axis =1))
            plt.pause(0.002)
        elif frame.type == coreG3FrameType.Housekeeping:
            pass

#need screen geometry and squid list and squid mapping
def add_squid_info(screen, y, x, 
                   sq_label, sq_label_size,
                   carrier_good, nuller_good, demod_good,
                   temperature_good, max_size,
                   bolometer_good, bolo_label = '',
                   neutral_c = 3, good_c = 2, bad_c = 1):
    assert( (6 + sq_label_size) < max_size)
    col_map = {True: curses.color_pair(good_c), 
               False: curses.color_pair(bad_c) }
    current_index = x
    screen.addstr(y, current_index, sq_label, curses.color_pair(neutral_c))
    current_index += sq_label_size

    screen.addstr(y, current_index, 'C', col_map[carrier_good])
    current_index += 1

    screen.addstr(y, current_index, 'N', col_map[nuller_good])
    current_index += 1

    screen.addstr(y, current_index, 'D', col_map[demod_good])
    current_index += 1

    screen.addstr(y, current_index, 'T', col_map[temperature_good])
    current_index += 1

    screen.addstr(y, current_index, 'B', col_map[bolometer_good])
    current_index += 1

    if (not bolometer_good):
        screen.addstr(y, 
                      current_index, ' '+bolo_label[:(max_size - 6 - sq_label_size )], 
                      col_map[False])

def load_squid_info_from_hk( screen, y, x, 
                             hk_map,
                             sq_dev_id, sq_label, sq_label_size, 
                             max_size, ip_mapper):
    carrier_good = False
    nuller_good = False 
    demod_good = False
    tv_good = False 
    bolometer_good = False
    bolo_label = 'NoData'

    #code for loading hk info for display
    if hk_map != None:
        board_id, mezz_num, module_num = sq_phys_id_to_info(sq_dev_id)
        board_ip = ip_mapper.get_ip(board_id)
        board_info = hk_map[board_id]
        module_info = hk_map[board_id][mezz_num][module_num]

        carrier_good = not module_info.carrier_railed
        nuller_good = not module_info.nuller_railed
        demod_good = not module_info.demod_railed

        #need to add temperature and voltage info
        tv_good = True
        #need to add bolometer info
        bolometer_good = True
        bolo_label = ''

    add_squid_info(screen, y, x, 
                   sq_label, sq_label_size,
                   carrier_good, nuller_good, demod_good,
                   tv_good, max_size,
                   bolometer_good, bolo_label)


class SquidDisplay(object):
    def __init__(self, squids_list, 
                 squids_per_col = 32, 
                 squid_col_width = 20):
        self.squids_list = squids_list
        self.squids_per_col = squids_per_col
        self.squid_col_width = squid_col_width
        self.n_squids = len(squids_list)

        self.sq_label_size = max(map(len, squids_list))        
        ncols = int(math.ceil(float(self.n_squids)/self.squids_per_col))

        self.screen_size_x = ncols * squid_col_width
        self.screen_size_y = self.squids_per_col + 2

        self.pos_map = {}
        #assign an x, y location to each squid

        self.ip_mapper = None
        for i, sq in enumerate(sorted(squids_list)):
            y =  i % self.squids_per_col + 1
            x = 1 + self.squid_col_width * ( i // self.squids_per_col)
            self.pos_map[sq] = (x,y)
        try:
            # Initialize curses
            self.stdscr = curses.initscr()

            y, x = self.stdscr.getmaxyx()
            if y < self.screen_size_y:
                raise RuntimeError("screen is not tall enough, extend to %d", self.screen_size_y)
            if x < self.screen_size_x:
                raise RuntimeError("screen is not wide enough, extend to %d", self.screen_size_x)

            #curses.mousemask(curses.ALL_MOUSE_EVENTS)
            curses.start_color()
            
            # Turn off echoing of keys, and enter cbreak mode,
            # where no buffering is performed on keyboard input
            curses.noecho()
            curses.cbreak()
            curses.curs_set(0)

            self.screen = self.stdscr.subwin(0, self.screen_size_x, 0, 0)

            curses.init_pair(1, curses.COLOR_RED,   curses.COLOR_WHITE)
            curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)
            curses.init_pair(3, curses.COLOR_BLUE,  curses.COLOR_BLACK)

        except:
            # In event of error, restore terminal to sane state.
            curses.curs_set(1)
            curses.echo()
            curses.nocbreak()
            curses.endwin()
            traceback.print_exc()  # Print the exception
        

    def __call__(self, frame):
        if frame == None or frame.type == core.G3FrameType.Housekeeping:
            #do update
            if frame != None:
                hk_data = frame['DfMuxHousekeeping']
            else:
                hk_data = None
            self.stdscr.clear()
            self.screen.box()

            for i, s in enumerate(self.squids_list):
                p = self.pos_map[s]
                load_squid_info_from_hk( self.screen, p[1], p[0], 
                                         hk_data,
                                         s, s, self.sq_label_size, 
                                         self.squid_col_width, self.ip_mapper)
            self.screen.refresh()
        elif frame.type == core.G3FrameType.EndProcessing:
            self.stdscr.keypad(0)
            curses.echo()
            curses.nocbreak()
            curses.endwin() 
        elif frame.type == core.G3FrameType.Wiring:
            self.ip_mapper = IdIpMapper(frame['WiringMap'])

if __name__=='__main__':
    try:
        chs = ['Ch%d'%i for i in range(256)]
        sqs = ['Sq%d'%i for i in range(256)]
        sd = SquidDisplay(sqs)
        fp = FocalPlaneStater(chs)
        while 1:
            plt.pause(0.002)
            sd(None)
    finally:
        curses.curs_set(1)
        curses.echo()
        curses.nocbreak()
        curses.endwin()


    '''
    ndets = 14705
    x = np.array(np.random.rand(ndets))
    y = np.array(np.random.normal(size = ndets))
    labels =  ['test_label_%s'%i for i in range( len(x) )]
    ugh = FancyScatterPlot(x,y, labels)
    plt.show(block=False)


    while 1:
        plt.pause(0.002)
        x = np.array(np.random.rand(ndets))
        y = np.array(np.random.normal(size = ndets))
        ugh.check_socket()
        ugh.update_data(x,y)
      main(stdscr)                    # Enter the main loop
      # Set everything back to normal
    except:
      # In event of error, restore terminal to sane state.
      stdscr.keypad(0)
      curses.echo()
      curses.nocbreak()
      curses.endwin()
      traceback.print_exc()           # Print the exception
    '''
