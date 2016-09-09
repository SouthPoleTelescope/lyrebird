import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec



class FancyScatterPlot(object):
    def mycall(self, event):
        self.event = event
        if event.xdata == None:
            return
        xsize = self.ax.get_xlim()[1]-self.ax.get_xlim()[0]
        ysize = self.ax.get_ylim()[1]-self.ax.get_ylim()[0]
        dist_perc =(((self.x-event.xdata)/xsize)**2 + ((self.y-event.ydata)/ysize)**2)**.5
        valid_inds = np.where( dist_perc < self.frac_screen)[0]
        self.highlight_inds(valid_inds, no_send = False)

    def __init__(self, x, y, labels, 
                 x_label = 'Test X Label',
                 y_label = 'Test Y Label',
                 send_port = 5555,
                 recv_port = 5556,
                 frac_screen = 0.03):
        self.set_labels(labels)
        self.x = x
        self.y = y
        self.frac_screen = frac_screen

        plt.ion()
        self.figure = plt.figure()
        self.figure.canvas.mpl_connect('button_press_event', self.mycall)

        self.gs = gridspec.GridSpec(3, 3)
        ax1 = plt.subplot(self.gs[0:2,0:2])
        self.ax = ax1
        self.main_plot, = plt.plot(x, y, '*')
        self.overplot, = plt.plot([],[], 'ro')
        self.x_label = x_label
        self.y_label = y_label
        plt.ylabel(self.y_label)

        self.update_hists()

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
                self.overplot.set_xdata( self.x[valid_inds] )
                self.overplot.set_ydata( self.y[valid_inds] )
        else:
            self.overplot.set_xdata( [] )
            self.overplot.set_ydata( [] )
        self.figure.canvas.draw()
        if not no_send:
            for i in valid_inds:
                self.sock_send.sendto(self.labels[i], ('127.0.0.1', self.send_port))

    def highlight_strs(self, strs, no_send = True):
        inds = []
        for s in strs:
            if s in self.label_map:
                inds.append(self.label_map[s])
        self.highlight_inds(inds, no_send = True)
            


class FocalPlaneStater(object):
    def __init__(self, 
                 channel_id_lst,
                 channel_dev_lst, 
                 noise_buffer_size = 128,
                 update_modulo = 1000
    ):
        assert(len(channel_id_lst) == len(channel_dev_lst))

        self.channel_id_lst = channel_id_lst
        self.channel_dev_lst = channel_dev_lst


        self.channel_map = {}
        for i, ch in enumerate(channel_dev_lst):
            self.channel_map[ch] = i

        self.noise_buffers = np.array([np.zeros( noise_buffer_size ) for ch in channel_dev_lst])
        self.current_val = np.array([0.0 for ch in channel_dev_lst])
        self.rnorm_conv = [1.0 for ch in channel_dev_lst]

        self.noise_buffer_ind = 0
        self.noise_buffer_size = noise_buffer_size
        self.id_serial_mapper = None

        self.update_modulo = update_modulo
        self.update_ind = 0
        
        self.plotter = FancyScatterPlot( x = self.current_val, 
                                         y = np.std(self.noise_buffers, axis =1),
                                         labels = self.channel_id_lst, 
                                         x_label = '$R_{frac}$',
                                         y_label = 'Noise' )
        plt.show(block = False)

    def __call__(self, frame):
        if frame.type == core.G3FrameType.Wiring:
            self.id_serial_mapper = IdSerialMapper(frame['WiringMap'])
        elif frame.type == core.G3FrameType.Timepoint:
            print 'timepoint'
            if self.id_serial_mapper == None:
                return

            self.plotter.check_socket()

            meta_samp = frame['DfMux']
            for serial in meta_samp.keys():
                board_id = self.id_serial_mapper.get_id(serial)
                for module_id in meta_samp[serial].keys():
                    samp = meta_samp[serial][module_id]
                    for i in range(len(samp)):
                        cid = '%s/%d/%d' % (board_id, module_id + 1, i + 1)
                        if cid in self.channel_map:
                            ind = self.channel_map[cid]
                            sval = samp[i*2]
                            self.noise_buffers[ind][self.noise_buffer_ind] = sval
                            if sval != 0:
                                #print ind, self.rnorm_conv[ind]
                                self.current_val[ind] = self.rnorm_conv[ind]/sval
                            else:
                                self.current_val[ind] = 0

            self.noise_buffer_ind = (self.noise_buffer_ind + 1) % self.noise_buffer_size
            if self.update_ind % self.update_modulo == 0:
                self.plotter.update_data(self.current_val, np.std(self.noise_buffers, axis =1))
            plt.pause(0.002)
        elif frame.type == core.G3FrameType.Housekeeping:
            print 'hk'
            hk_data = frame['DfMuxHousekeeping']
            for serial in hk_data.keys():
                board_id = self.id_serial_mapper.get_id(serial)
                for mezz in hk_data[serial].mezz.keys():
                    for module in hk_data[serial].mezz[mezz].modules.keys():
                        for ch in hk_data[serial].mezz[mezz].modules[module].channels.keys():
                            mod = mezz * 4 + module
                            label = '%s/%d/%d' % (board_id, mod, ch)
                            if label in self.channel_map:
                                chk = hk_data[serial].mezz[mezz].modules[module].channels[ch]
                                ind = self.channel_map[label]
                                rn = 0 if abs(chk.rnormal) < 1e-12 else chk.res_conversion_factor / chk.rnormal
                                self.rnorm_conv[ind] = rn
